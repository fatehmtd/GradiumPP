#include <CLI/CLI.hpp>
#include <gradiumpp/gradium.hpp>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[])
{
    const char* apiKeyEnv = std::getenv("GRADIUM_API_KEY");
    if (apiKeyEnv == nullptr) {
        std::cerr << "Error: GRADIUM_API_KEY is not set\n";
        return 1;
    }

    std::string filePath;
    std::string inputFormat = "wav";
    std::string modelName   = "default";
    std::string language;

    CLI::App app{"Transcribe an audio file using the Gradium ASR REST API"};
    app.add_option("--file",   filePath,    "Path to audio file")->required();
    app.add_option("--format", inputFormat, "Audio input format (wav, pcm, opus, ...)");
    app.add_option("--model",  modelName,   "ASR model name");
    app.add_option("--lang",   language,    "Language hint (en, fr, de, es, pt)");
    CLI11_PARSE(app, argc, argv);

    gradium::AsrRestClient client(apiKeyEnv);

    gradium::AsrConfig config;
    config.input_format  = inputFormat;
    config.model_name    = modelName;
    config.json_config.language = language;

    try {
        const std::string result = client.transcribeFile(filePath, config);
        std::cout << result << "\n";
    } catch (const gradium::GradiumApiException& ex) {
        std::cerr << "API error: " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
