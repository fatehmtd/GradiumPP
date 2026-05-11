#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <cppcodec/base64_rfc4648.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>

int main(int argc, char* argv[])
{
    const char* apiKeyEnv = std::getenv("GRADIUM_API_KEY");
    if (apiKeyEnv == nullptr) {
        std::cerr << "Error: GRADIUM_API_KEY is not set\n";
        return 1;
    }

    std::string voiceId;
    CLI::App app{"Multiplex three TTS streams over one WebSocket connection"};
    app.add_option("--voice", voiceId, "Voice UID for all streams")->required();
    CLI11_PARSE(app, argc, argv);

    // Three streams, each writing to a separate file
    const std::vector<std::string> reqIds   = {"req-1", "req-2", "req-3"};
    const std::vector<std::string> texts    = {
        "Hello from stream one.",
        "Greetings from stream two.",
        "Welcome from the third stream."
    };
    const std::vector<std::string> outPaths = {
        "tts_multiplex_1.wav",
        "tts_multiplex_2.wav",
        "tts_multiplex_3.wav"
    };

    std::map<std::string, std::ofstream> outputs;
    for (std::size_t i = 0; i < reqIds.size(); ++i) {
        outputs[reqIds[i]].open(outPaths[i], std::ios::binary);
        if (!outputs[reqIds[i]].is_open()) {
            std::cerr << "Error: cannot open " << outPaths[i] << "\n";
            return 1;
        }
    }

    std::atomic<int>        finished{0};
    std::mutex              doneMutex;
    std::condition_variable doneCv;
    const int               totalStreams = static_cast<int>(reqIds.size());

    gradium::TtsRealtimeClient client;

    client.setOnParsedMessage([&](const gradium::TtsRealtimeMessage& msg) {
        const std::string& id = msg.client_req_id;

        if (!msg.audio_base64.empty() && outputs.count(id)) {
            auto bytes = cppcodec::base64_rfc4648::decode(msg.audio_base64);
            outputs[id].write(reinterpret_cast<const char*>(bytes.data()),
                              static_cast<std::streamsize>(bytes.size()));
        }

        if (msg.type == "end_of_stream" && !id.empty()) {
            outputs[id].close();
            if (finished.fetch_add(1) + 1 == totalStreams) {
                std::lock_guard<std::mutex> lk(doneMutex);
                doneCv.notify_all();
            }
        }

        if (msg.type == "error") {
            std::cerr << "[" << id << "] TTS error: " << msg.error_message << "\n";
        }
    });

    client.setOnError([&](const std::string& err) {
        std::cerr << "WebSocket error: " << err << "\n";
        std::lock_guard<std::mutex> lk(doneMutex);
        doneCv.notify_all();
    });

    try {
        client.connect(apiKeyEnv);

        // Set up all three streams — keep connection open for multiple EOS
        for (const auto& reqId : reqIds) {
            gradium::TtsRealtimeSetup setup;
            setup.voice_id        = voiceId;
            setup.output_format   = "wav";
            setup.client_req_id   = reqId;
            setup.close_ws_on_eos = false;
            client.setup(setup);
        }

        // Send text for all three
        for (std::size_t i = 0; i < reqIds.size(); ++i) {
            client.sendText(texts[i], reqIds[i]);
            client.sendEndOfStream(reqIds[i]);
        }

        // Wait for all to finish
        {
            std::unique_lock<std::mutex> lk(doneMutex);
            doneCv.wait(lk, [&] { return finished.load() == totalStreams; });
        }

        client.close();

        for (std::size_t i = 0; i < reqIds.size(); ++i) {
            std::cout << "Wrote multiplex stream " << reqIds[i] << " to " << outPaths[i] << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "TTS multiplex failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
