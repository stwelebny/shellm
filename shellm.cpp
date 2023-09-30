
/* Copyright (c) 2023 Stefan Welebny
**
** This file is part of [shellm], licensed under the MIT License.
** For full terms see the included LICENSE file.
**
**/

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <curl/curl.h>
#include <vector>
#include <utility> // for std::pair
#include <json/json.h>  // For jsoncpp
#include <sstream>  // For std::istringstream
#include <unistd.h>
#include<limits> //used to get numeric limits

Json::Value loadJSONFromFile(const std::string& filename);

size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string chatWithGPT(const std::vector<std::pair<std::string, std::string> >& messages, const std::string& api_key, const std::string& modelName, int max_tokens, int total_tokens_used) {
    // Initialize CURL
    CURL *curl = curl_easy_init();
    std::string response;

    if(curl) {
        // Clone the original messages vector
        auto temp_messages = messages;

        // Use the total_tokens field from the previous API response to determine remaining tokens
        int remaining_tokens = max_tokens/2 - total_tokens_used;


        // Remove messages from the beginning until we fit within the token limit
        while (remaining_tokens < 0 && temp_messages.size() > 2) {
            int tokens_to_remove = temp_messages.front().second.length()/5;  // Estimate tokens in the first message
            remaining_tokens += tokens_to_remove;
            temp_messages.erase(temp_messages.begin());  // Remove the first message
        }

        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        Json::Value messagesJson(Json::arrayValue);  // Create a JSON array

	for (const auto& message : temp_messages) {
	    Json::Value msg(Json::objectValue);  // Create a JSON object for each message
	    msg["role"] = message.first;
	    msg["content"] = message.second;
	    messagesJson.append(msg);  // Append the message object to the array
	}

	Json::Value root(Json::objectValue);  // Create the root JSON object
	root["model"] = modelName;
	root["messages"] = messagesJson;
        Json::Value tools =  loadJSONFromFile("functions.def");
        root["functions"] = tools;       

	Json::StreamWriterBuilder writer;
	std::string post_fields = Json::writeString(writer, root);  // Convert the JSON object to a string
//        std::cout << post_fields + "\n";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

	// Set the write callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

     // Perform the request
        CURLcode res = curl_easy_perform(curl);

        // Cleanup
        curl_easy_cleanup(curl);
    }

    return response;
}

bool endsWith(const std::string& mainStr, const std::string& toMatch) {
    // Check if the toMatch string is longer than the main string
    if (mainStr.size() >= toMatch.size() &&
        mainStr.rfind(toMatch) == mainStr.size() - toMatch.size()) {
        return true;
    }
    return false;
}


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Function to execute a Linux command based on the function specification
Json::Value executeLinuxCommand(const std::string& arguments_str) {
    // Parse function specification to get required parameters
    Json::Value arguments_json;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(arguments_str, arguments_json);
    if (!parsingSuccessful) {
        // Handle parsing failure (e.g., return an error JSON object)
    }

    // Extract the command from the parsed arguments
    std::string command = arguments_json["command"].asString();
    // Call the exec function
    std::string output = exec(command.c_str());

    // Format the output into a JSON object
    Json::Value outputJson;
    outputJson["tool"] = "linux_command";
    outputJson["command"] = command;
    outputJson["output"] = output;

    return outputJson;
}

Json::Value executeSelfCallCommand(const std::string& arguments_str) {
    // Parse function specification to get required parameters
    Json::Value arguments_json;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(arguments_str, arguments_json);
    if (!parsingSuccessful) {
        // Handle parsing failure (e.g., return an error JSON object)
    }
    std::string command = arguments_json["task"].asString();

    // Format the output into a JSON object
    Json::Value outputJson;
    outputJson["tool"] = "self_call";
    outputJson["command"] = command;
    outputJson["output"] = command;

    return outputJson;
}

Json::Value executeDummyCommand(const std::string& arguments_str) {

    // Format the output into a JSON object
    Json::Value outputJson;
    outputJson["tool"] = "unknow tool";
    outputJson["output"] = "This tool is not implemented";

    return outputJson;
}



std::map<std::string, std::string> readConfigFile(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        if (line[0] == '#' || line.empty()) continue; // Skip comments and empty lines

        size_t pos = line.find("=");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            config[key] = value;
        }
    }

    return config;
}

// Function to save JSON to a file
void saveJSONToFile(const Json::Value& root, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << root;
        file.close();
    }
}

// Function to load JSON from a file
Json::Value loadJSONFromFile(const std::string& filename) {
    std::ifstream file(filename);
    Json::Value root;
    if (file.is_open()) {
        file >> root;
        file.close();
    }
    return root;
}

// Function to save messages to a file
void saveMessagesToFile(const std::vector<std::pair<std::string, std::string>>& messages, const std::string& filename) {
    Json::Value messageArray(Json::arrayValue);
    for (const auto& message : messages) {
        Json::Value jsonMessage;
        jsonMessage["role"] = message.first;
        jsonMessage["content"] = message.second;
        messageArray.append(jsonMessage);
    }
    saveJSONToFile(messageArray, filename);
}

// Function to load messages from a file
std::vector<std::pair<std::string, std::string>> loadMessagesFromFile(const std::string& filename) {
    Json::Value loadedRoot = loadJSONFromFile(filename);
    std::vector<std::pair<std::string, std::string>> loadedMessages;
    for (const auto& message : loadedRoot) {
        loadedMessages.push_back({message["role"].asString(), message["content"].asString()});
    }
    return loadedMessages;
}

// Function to convert messages vector to chatHistory string
std::string messagesToChatHistory(const std::vector<std::pair<std::string, std::string>>& messages) {
    std::string chatHistory;
    for (const auto& message : messages) {
        chatHistory += message.first + ": " + message.second + "\n";
    }
    return chatHistory;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    std::string filename = argv[1];
    // Load existing chat history
    std::vector<std::pair<std::string, std::string>> messages = loadMessagesFromFile(filename);
    // Initialization
    if (messages.empty()) {
        messages.push_back({"system", "You are a helpful assistant."});
    }
    std::string chatHistory = messagesToChatHistory(messages);
    std::cout << chatHistory << std::endl;

    std::map<std::string, std::string> config = readConfigFile(".chat.conf");
    std::string apiKey = config["API_KEY"];
    std::string modelName = config["MODEL_NAME"];
    int max_tokens = 0;
    try {
        max_tokens = std::stoi(config["MAX_TOKENS"]);
    } catch (const std::invalid_argument& e) {
    } catch (const std::out_of_range& e) {
    }

    int total_tokens = max_tokens;
    std::string tokenInfo="";
    std::string gpt_response = "";
    std::string json_response = "";

    bool isInteractive = isatty(fileno(stdin));

    // Main chat loop
    while (1) {
        std::cin.clear();
        std::cout << "Send an enquiry (submit with ctrl-E):" << std::endl;
        std::string user_input="";
        std::string line="";
	if (gpt_response.find("{continue?}") != std::string::npos) {
          user_input = "Please continue!";
	  std::cout << user_input << std::endl;
	}
        else {
          char ch;
          while (std::cin.get(ch)) {  // read character by character
            if (ch == '\x05') {  // stop on CTRL-E
              break;
            }
            user_input += ch;
          }
          /* std::cin.ignore();
          std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          // std::getline(std::cin, user_input);
          std::getline(std::cin, user_input, '\x05');  // Using CTRL-E as the delimiter
          // Check if the last character is CTRL-E and remove it
          if (!user_input.empty() && user_input.back() == '\x05') {
            user_input.pop_back();
          } */
	}
	messages.push_back({"user", user_input});
	// Get ChatGPT response and parse it
	json_response = chatWithGPT(messages,apiKey, modelName, max_tokens, total_tokens);
	Json::Value root;
	std::istringstream json_stream(json_response);
	json_stream >> root;
	gpt_response = root["choices"][0]["message"]["content"].asString();
	total_tokens = root["usage"]["total_tokens"].asInt();
        int i=0;
        while (root["choices"][0]["message"].isMember("function_call") && i < 10) {
            std::string functionName = root["choices"][0]["message"]["function_call"]["name"].asString();
	    std::string arguments = root["choices"][0]["message"]["function_call"]["arguments"].asString();
            messages.push_back({"assistant", functionName + " " + arguments});
            std::cout << "assistant: " + functionName + " " + arguments << std::endl;
            Json::Value result; 
            if (functionName == "execute_linux_command") {
               result = executeLinuxCommand(arguments);
	    } else if (functionName == "self_call") {
	       result = executeSelfCallCommand(arguments);
            } else {
	      result = executeDummyCommand(arguments);
	    }
	    Json::StreamWriterBuilder writer;
            std::string resultString = Json::writeString(writer, result);  // Convert the JSON object to a string
            messages.push_back({"system", resultString});
            std::cout << "system: " + resultString << std::endl;
	    std::string json_response = chatWithGPT(messages,apiKey, modelName, max_tokens, total_tokens);
            std::istringstream json_stream(json_response);
            json_stream >> root;
	    total_tokens = root["usage"]["total_tokens"].asInt();
	    i++;
        }
	if (!gpt_response.empty()) { 
	    messages.push_back({"assistant", gpt_response});
            std::cout << "assistant: " + gpt_response << std::endl;
	}
        saveMessagesToFile(messages, filename);
        Json::Value usage = root["usage"];
	int prompt_tokens = usage["prompt_tokens"].asInt();
	int completion_tokens = usage["completion_tokens"].asInt();
        total_tokens = root["usage"]["total_tokens"].asInt();
	tokenInfo = "Prompt: " + std::to_string(prompt_tokens) + 
		", Completion: " + std::to_string(completion_tokens) + 
		", Total Tokens: " + std::to_string(total_tokens) + "\n";
        std::cout << tokenInfo + "\n";
        if (!isInteractive)
		break;
    }
    return 0;
}
