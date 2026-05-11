#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    const char* apiKeyEnv = std::getenv("GRADIUM_API_KEY");
    if (apiKeyEnv == nullptr) {
        std::cerr << "Error: GRADIUM_API_KEY is not set\n";
        return 1;
    }

    bool        listOnly   = false;
    std::string createFile;
    std::string createName;

    CLI::App app{"List voices and run a basic create-update-delete flow"};
    app.add_flag  ("--list-only",    listOnly,   "Only list voices and credits");
    app.add_option("--create-file",  createFile, "Audio file for custom voice creation");
    app.add_option("--create-name",  createName, "Name for the custom voice");
    CLI11_PARSE(app, argc, argv);

    gradium::VoiceClient   voiceClient(apiKeyEnv);
    gradium::CreditsClient creditsClient(apiKeyEnv);

    try {
        const auto credits = creditsClient.getCreditsTyped();
        std::cout << "=== Credits ===\n";
        std::cout << "  Plan:      " << credits.plan_name          << "\n";
        std::cout << "  Remaining: " << credits.remaining_credits  << "\n";
        std::cout << "  Allocated: " << credits.allocated_credits  << "\n";
        std::cout << "  Period:    " << credits.billing_period      << "\n\n";

        const auto voices = voiceClient.listVoicesTyped(false);
        std::cout << "=== Your Voices (" << voices.size() << ") ===\n";
        for (const auto& v : voices) {
            std::cout << "  [" << v.uid << "]  " << v.name;
            if (!v.language.empty()) std::cout << "  lang=" << v.language;
            if (v.is_pro_clone)      std::cout << "  (pro clone)";
            std::cout << "\n";
        }

        if (listOnly || createFile.empty() || createName.empty()) {
            return 0;
        }

        std::cout << "\n=== Creating voice: " << createName << " ===\n";
        gradium::VoiceCreateRequest createReq;
        createReq.name            = createName;
        createReq.audio_file_path = createFile;
        createReq.description     = "Created from the GradiumPP voices example";

        const auto newVoice = voiceClient.createVoiceTyped(createReq);
        std::cout << "Created voice: " << newVoice.uid << "  \"" << newVoice.name << "\"\n";

        gradium::VoiceUpdateRequest updateReq;
        updateReq.name = createName + " (updated)";
        const auto updated = voiceClient.updateVoiceTyped(newVoice.uid, updateReq);
        std::cout << "Updated voice name to: \"" << updated.name << "\"\n";

        voiceClient.deleteVoice(newVoice.uid);
        std::cout << "Deleted voice: " << newVoice.uid << "\n";

    } catch (const gradium::GradiumApiException& ex) {
        std::cerr << "API error: " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
