#include "dds/Publisher.h"

#include <unitree/idl/go2/MotorCmds_.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace
{

constexpr int kFingerCount = 6;
constexpr float kDefaultSpeed = 1.0f;

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

void print_usage(const char* program)
{
    std::cout
        << "Usage:\n"
        << "  sudo " << program << " <left|right|both> <finger_index|all|both> <position|open|close> [speed]\n"
        << "  sudo " << program << " <left|right|both> <open|close> [speed]\n\n"
        << "Finger index: 0~5 = [Thumb, ThumbAux, Index, Middle, Ring, Pinky]\n"
        << "Position: 0.0=open, 1.0=closed, middle values are allowed\n"
        << "Speed: 0.0~1.0, default is 1.0\n\n"
        << "Examples:\n"
        << "  sudo " << program << " left 3 0.5 1\n"
        << "  sudo " << program << " right both open\n"
        << "  sudo " << program << " both close\n"
        << "  sudo " << program << " both all 0.3 0.8\n";
}

std::optional<float> parse_float(const std::string& text)
{
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0') {
        return std::nullopt;
    }
    return value;
}

std::optional<float> parse_position(const std::string& text)
{
    const auto token = lower(text);
    if (token == "open") {
        return 0.0f;
    }
    if (token == "close" || token == "closed") {
        return 1.0f;
    }

    auto value = parse_float(text);
    if (!value || *value < 0.0f || *value > 1.0f) {
        return std::nullopt;
    }
    return *value;
}

std::optional<float> parse_speed(const std::string& text)
{
    auto value = parse_float(text);
    if (!value || *value < 0.0f || *value > 1.0f) {
        return std::nullopt;
    }
    return *value;
}

std::optional<int> parse_finger_index(const std::string& text)
{
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || value < 0 || value >= kFingerCount) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

bool is_all_fingers_token(const std::string& text)
{
    const auto token = lower(text);
    return token == "all" || token == "both";
}

bool add_hands(const std::string& hand_arg, std::vector<std::string>& hands)
{
    const auto hand = lower(hand_arg);
    if (hand == "left") {
        hands.push_back("left");
        return true;
    }
    if (hand == "right") {
        hands.push_back("right");
        return true;
    }
    if (hand == "both") {
        hands.push_back("left");
        hands.push_back("right");
        return true;
    }
    return false;
}

struct Command
{
    std::vector<std::string> hands;
    std::vector<int> fingers;
    float position{kDefaultSpeed};
    float speed{kDefaultSpeed};
};

std::optional<Command> parse_command(int argc, char** argv)
{
    if (argc < 3 || argc > 5) {
        return std::nullopt;
    }

    Command command;
    if (!add_hands(argv[1], command.hands)) {
        std::cerr << "Invalid hand selector: " << argv[1] << "\n";
        return std::nullopt;
    }

    if (argc == 3 || (argc == 4 && !is_all_fingers_token(argv[2]) && parse_position(argv[2]))) {
        const auto position = parse_position(argv[2]);
        if (!position) {
            std::cerr << "Invalid position/action: " << argv[2] << "\n";
            return std::nullopt;
        }
        command.position = *position;
        if (argc == 4) {
            const auto speed = parse_speed(argv[3]);
            if (!speed) {
                std::cerr << "Invalid speed: " << argv[3] << "\n";
                return std::nullopt;
            }
            command.speed = *speed;
        }
        for (int i = 0; i < kFingerCount; ++i) {
            command.fingers.push_back(i);
        }
        return command;
    }

    if (argc < 4) {
        return std::nullopt;
    }

    if (is_all_fingers_token(argv[2])) {
        for (int i = 0; i < kFingerCount; ++i) {
            command.fingers.push_back(i);
        }
    } else {
        const auto finger = parse_finger_index(argv[2]);
        if (!finger) {
            std::cerr << "Invalid finger index: " << argv[2] << "\n";
            return std::nullopt;
        }
        command.fingers.push_back(*finger);
    }

    const auto position = parse_position(argv[3]);
    if (!position) {
        std::cerr << "Invalid position/action: " << argv[3] << "\n";
        return std::nullopt;
    }
    command.position = *position;

    if (argc == 5) {
        const auto speed = parse_speed(argv[4]);
        if (!speed) {
            std::cerr << "Invalid speed: " << argv[4] << "\n";
            return std::nullopt;
        }
        command.speed = *speed;
    }

    return command;
}

void publish_hand_command(const std::string& hand, const std::vector<int>& fingers, float position, float speed)
{
    auto publisher = std::make_unique<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>(
        "rt/brainco/" + hand + "/cmd");
    publisher->msg_.cmds().resize(kFingerCount);

    for (auto& finger : publisher->msg_.cmds()) {
        finger.dq() = kDefaultSpeed;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (int repeat = 0; repeat < 3; ++repeat) {
        publisher->lock();
        for (int index : fingers) {
            publisher->msg_.cmds()[index].q() = position;
            publisher->msg_.cmds()[index].dq() = speed;
        }
        publisher->unlockAndPublish();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2) {
        const auto arg = lower(argv[1]);
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    const auto command = parse_command(argc, argv);
    if (!command) {
        print_usage(argv[0]);
        return 1;
    }

    unitree::robot::ChannelFactory::Instance()->Init(0, "");

    for (const auto& hand : command->hands) {
        publish_hand_command(hand, command->fingers, command->position, command->speed);
        std::cout << "Sent " << hand << " command: fingers";
        for (int index : command->fingers) {
            std::cout << ' ' << index;
        }
        std::cout << ", position " << command->position << ", speed " << command->speed << "\n";
    }

    return 0;
}
