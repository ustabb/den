#include <iostream>
#include <memory>
#include <vector>
#include "control/config_manager.hpp"
#include "streaming/publisher.hpp"
#include "streaming/subscriber.hpp"
#include "media/encoder.hpp"
#include "media/decoder.hpp"
#include "media/codecs.hpp"
#include "network/udp_server.hpp"

using namespace std;

int main() {
    cout << "Stream Engine Starting..." << endl;

    // Config yükle
    control::ConfigManager config;
    if (!config.load("config.json")) {
        cerr << "Failed to load config" << endl;
        return 1;
    }

    // Network bileşenleri
    auto udp_server = make_unique<network::BasicUDPServer>();
    if (!udp_server->start(8080)) {
        cerr << "Failed to start UDP server" << endl;
        return 1;
    }

    // Media bileşenleri
    auto encoder = make_unique<media::RLEEncoder>();
    auto decoder = make_unique<media::RLEDecoder>();

    // Streaming bileşenleri
    streaming::Publisher publisher(move(encoder), move(udp_server));
    
    // Subscriber için yeni bir UDP server
    auto subscriber_server = make_unique<network::BasicUDPServer>();
    subscriber_server->start(8081);
    streaming::Subscriber subscriber(move(decoder), move(subscriber_server));

    cout << "Stream Engine Started Successfully" << endl;

    // Test verisi
    vector<uint8_t> test_frame = {0x01, 0x02, 0x03, 0x04, 0x05};

    // Publisher test
    if (publisher.publish_frame(test_frame)) {
        cout << "Frame published successfully" << endl;
    } else {
        cerr << "Failed to publish frame" << endl;
    }

    // Subscriber test
    auto received_frame = subscriber.receive_frame();
    if (!received_frame.empty()) {
        cout << "Frame received, size: " << received_frame.size() << " bytes" << endl;
    } else {
        cout << "No frame received" << endl;
    }

    cout << "Stream Engine Stopping..." << endl;
    return 0;
}