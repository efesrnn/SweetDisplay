// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#include "../protocol/TcpStream.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace P = SweetDisplay::Protocol;
namespace T = SweetDisplay::Transport;

namespace {
constexpr uint16_t kPort = 48231;
constexpr uint32_t kHeartbeatsPerSession = 8;

struct SessionResult {
    std::string local;
    std::string peer;
    uint64_t bytesWritten = 0;
    uint64_t bytesRead = 0;
    uint64_t messagesWritten = 0;
    uint64_t messagesRead = 0;
};

std::vector<uint8_t> Token(uint32_t sessionOrdinal, uint32_t messageOrdinal) {
    std::vector<uint8_t> payload(8);
    const uint64_t token = 0xE200000000000000ULL
        | (uint64_t(sessionOrdinal) << 32)
        | uint64_t(messageOrdinal);
    P::Put64(payload.data(), token);
    return payload;
}

SessionResult RunSession(const char* phoneAddress, const std::string& expectedLocal,
                         uint32_t sessionOrdinal) {
    auto stream = T::TcpStream::ConnectAddress(phoneAddress, kPort);
    SessionResult result;
    result.local = stream->LocalAddress();
    result.peer = stream->PeerAddress();
    P::Require(result.local.rfind(expectedLocal + ":", 0) == 0,
               "unexpected local endpoint");
    P::Require(result.peer == std::string(phoneAddress) + ":" + std::to_string(kPort),
               "unexpected peer endpoint");

    T::Channel channel(*stream);
    P::Connection connection(P::Role::Host, T::SessionId());
    T::HostHandshake(channel, connection);

    for (uint32_t i = 1; i <= kHeartbeatsPerSession; ++i) {
        auto payload = Token(sessionOrdinal, i);
        const auto deadline = T::Now() + 2000000000ULL;
        channel.Send(connection.Make(P::Type::Heartbeat, payload, T::Now()), deadline);
        auto echo = channel.Receive(deadline);
        connection.Receive(echo);
        P::Require(echo.header.type == P::Type::Heartbeat, "heartbeat echo type");
        P::Require(echo.payload == payload, "heartbeat payload integrity");
    }

    std::vector<uint8_t> drain(8);
    P::Put32(drain.data(), 1);
    const auto deadline = T::Now() + 2000000000ULL;
    channel.Send(connection.Make(P::Type::Control, drain, T::Now()), deadline);
    auto acknowledgement = channel.Receive(deadline);
    connection.Receive(acknowledgement);
    P::Require(acknowledgement.header.type == P::Type::Control,
               "drain acknowledgement type");
    P::Require(P::U32(acknowledgement.payload.data()) == 2
                   && P::U32(acknowledgement.payload.data() + 4) == 0,
               "drain acknowledgement payload");
    P::Require(!channel.Partial(), "partial message at clean close");

    result.bytesWritten = channel.bytesWritten;
    result.bytesRead = channel.bytesRead;
    result.messagesWritten = channel.messagesWritten;
    result.messagesRead = channel.messagesRead;
    return result;
}

void WriteJson(const char* path, const std::array<SessionResult, 2>& sessions) {
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output) throw std::runtime_error("result file open failed");
    uint64_t totalBytes = 0;
    for (const auto& session : sessions) {
        totalBytes += session.bytesWritten + session.bytesRead;
    }
    output << "{\n"
           << "  \"outcome\": \"PASS\",\n"
           << "  \"transport\": \"direct-ipv4-tcp\",\n"
           << "  \"framing\": \"SWDP-48-byte-header\",\n"
           << "  \"sessions\": [\n";
    for (size_t i = 0; i < sessions.size(); ++i) {
        const auto& session = sessions[i];
        output << "    {\"ordinal\": " << (i + 1)
               << ", \"local\": \"" << session.local
               << "\", \"peer\": \"" << session.peer
               << "\", \"heartbeats\": " << kHeartbeatsPerSession
               << ", \"messages_written\": " << session.messagesWritten
               << ", \"messages_read\": " << session.messagesRead
               << ", \"bytes_written\": " << session.bytesWritten
               << ", \"bytes_read\": " << session.bytesRead << "}";
        output << (i + 1 == sessions.size() ? "\n" : ",\n");
    }
    output << "  ],\n"
           << "  \"reconnects\": 1,\n"
           << "  \"total_wire_bytes\": " << totalBytes << ",\n"
           << "  \"payload_integrity\": true,\n"
           << "  \"sequence_integrity\": true,\n"
           << "  \"clean_drains\": 2,\n"
           << "  \"crc_exercised\": false,\n"
           << "  \"video_exercised\": false\n"
           << "}\n";
    if (!output) throw std::runtime_error("result file write failed");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 4) {
            std::cerr << "usage: NcmProtocolProbe PHONE_IPV4 EXPECTED_LOCAL_IPV4 RESULT_JSON\n";
            return 2;
        }
        T::Winsock winsock;
        std::array<SessionResult, 2> sessions{};
        sessions[0] = RunSession(argv[1], argv[2], 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        sessions[1] = RunSession(argv[1], argv[2], 2);
        WriteJson(argv[3], sessions);
        std::cout << "PASS: direct NCM TCP protocol exchange, one reconnect\n";
        return 0;
    } catch (const T::IoError& error) {
        std::cerr << "ERROR: " << error.what() << " winsock=" << error.code << "\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << "\n";
        return 1;
    }
}
