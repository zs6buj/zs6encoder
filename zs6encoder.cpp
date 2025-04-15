/*
    Port of duckencoder.jar to MSVC C++ by Eric Stockenstrom  2025-04-15
    
    Acknowledgements:
        Hak5 Darren - the master
        Fork: https://github.com/netscylla/Ducky-Encoder

    USAGE:

        Clone or copy to {YourFolder}
        Navigate to {YourFolder}
        Create/Edit "payload.txt" input file
        Launch launchDevCmd.bat or regular Command Shell
        Launch zs6encoder.exe (or zs6encoder .) 
        Your new inject.bin is created in {YourFolder}
        Check the monitor output for errors

*/
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

class encoder 
{
private:
    static std::string version;
    static bool debug;

    static void trim(std::string& s) 
    {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    }

    static void printKeyboardProps()
    {
        std::cout << "\nPrint keyboardProps"  << std::endl;
        for (auto it = keyboardProps.begin(); it != keyboardProps.end(); ++it) 
        {
            std::cout << "[" << it->first << "] => " << it->second << std::endl;
        }
        std::cout << std::endl;   
    }

    static void printLayoutProps()
    {
        std::cout << "\nPrint layoutProps"  << std::endl;
        for (auto it = layoutProps.begin(); it != layoutProps.end(); ++it) 
        {
            std::cout << "[" << it->first << "] => " << it->second << std::endl;
        }
        std::cout << std::endl;
    }
public:

    static std::map<std::string, std::string> keyboardProps;
    static std::map<std::string, std::string> layoutProps;  // container
    static void setup() {
        loadProperties("us");
    }

    static void loop() {
        std::string inputFile = "payload.txt";
        std::string outputFile = "inject.bin";
        std::string layoutFile = "layout.properties";

        std::string scriptStr = loadFile(inputFile);
        encodeToFile(scriptStr, outputFile);
    }

    static std::string loadFile(const std::string& path) 
    {
        std::ifstream file(path);
        if (!file.is_open()) 
        {
            std::cerr << "Failed to input open file" << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static void loadProperties(const std::string& lang) 
    {
        std::ifstream file("resources/keyboard.properties");
        if (!file.is_open()) 
        {
            std::cerr << "Failed to open keyboard.properties" << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) 
        {
            size_t separatorIndex = line.find('=');
            if (separatorIndex != std::string::npos) {
                std::string key = line.substr(0, separatorIndex);
                std::string value = line.substr(separatorIndex + 1);
                trim(key);
                trim(value);               
                keyboardProps[key] = value;
            }
        }
        file.close();

        //printKeyboardProps();

        file.open("resources/" + lang + ".properties");
        if (!file.is_open()) {
            std::cerr << "Failed to open " << lang << ".properties" << std::endl;
            return;
        }

        while (std::getline(file, line)) 
        {
            size_t separatorIndex = line.find('=');
            if (separatorIndex != std::string::npos) // npos is largest value
            {
                std::string key = line.substr(0, separatorIndex);           
                std::string value = line.substr(separatorIndex + 1);
                //std::cout << "ejs getline() key:" << key  << std::endl;                  
                //std::cout << "ejs getline() value:" << value  << std::endl;  
                trim(key);
                trim(value);  
                layoutProps[key] = value;               
            }
        }
        file.close();
        
        //printLayoutProps();
    }

    static void encodeToFile(const std::string& inStr, const std::string& fileDest) 
    {
        std::string lines = inStr;
        std::vector<uint8_t> file;
        int defaultDelay = 0;
        bool delayOverride = false;

        std::istringstream stream(lines);
        std::string line;
        while (std::getline(stream, line)) 
        {
            std::cout << "line: " << line << std::endl;
            if (line.empty() || line.substr(0, 3) == "REM") continue;

            if (line.substr(0, 13) == "DEFAULT_DELAY" || line.substr(0, 12) == "DEFAULTDELAY") 
            {
                defaultDelay = std::stoi(line.substr(line.find(' ') + 1));
                delayOverride = true;
            } else if (line.substr(0, 5) == "DELAY") 
            {
                int delay = std::stoi(line.substr(line.find(' ') + 1));
                while (delay > 0) {
                    file.push_back(0x00);
                    if (delay > 255) 
                    {
                        file.push_back(0xFF);
                        delay -= 255;
                    } else 
                    {
                        file.push_back(delay);
                        delay = 0;
                    }
                }
                delayOverride = true;
            } else if (line.substr(0, 6) == "STRING") 
            {
                std::string str = line.substr(line.find(' ') + 1);
                for (char c : str) 
                {  
                    //std::cout << "ejs c: " << c << std::endl;
                    addBytes(file, charToBytes(c));
                }
            } else 
            {
                // Handle other lines similarly
            }

            if (!delayOverride && defaultDelay > 0) 
            {
                int delayCounter = defaultDelay;
                while (delayCounter > 0) 
                {
                    file.push_back(0x00);
                    if (delayCounter > 255) 
                    {
                        file.push_back(0xFF);
                        delayCounter -= 255;
                    } else 
                    {
                        file.push_back(delayCounter);
                        delayCounter = 0;
                    }
                }
            }
        }

        std::ofstream outFile(fileDest, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file for writing" << std::endl;
            return;
        }

        outFile.write(reinterpret_cast<const char*>(file.data()), file.size());
        outFile.close();
    }

    static void addBytes(std::vector<uint8_t>& file, const std::vector<uint8_t>& byteTab) 
    {
        file.insert(file.end(), byteTab.begin(), byteTab.end());
        if (byteTab.size() % 2 != 0) 
        {
            file.push_back(0x00);
        }
    }

    static std::vector<uint8_t> charToBytes(char c) 
    {
        return codeToBytes(charToCode(c));
    }

    static std::string charToCode(char c) 
    {
        unsigned char uc = static_cast<unsigned char>(c);
        std::stringstream ss;
        if (uc < 128) 
        {
            ss << "ASCII_" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(uc);
        } else if (uc < 256) 
        {
            ss << "ISO_8859_1_" << std::uppercase << std::hex 
            << std::setw(2) << std::setfill('0') << static_cast<int>(uc);
        } else 
        {
            ss << "UNICODE_" << std::uppercase << std::hex 
            << static_cast<int>(uc);  // unlikely with just a char, but kept for consistency
        }
        return ss.str();
    }

    static std::vector<uint8_t> codeToBytes(const std::string& str) 
    {
        std::vector<uint8_t> byteTab;
        std::string trimmedStr = str;
        trim(trimmedStr);
        if (layoutProps.find(trimmedStr) != layoutProps.end()) 
        {
            std::istringstream keysStream(layoutProps[trimmedStr]);
            std::string key;
            while (std::getline(keysStream, key, ',')) 
            {
                trim(key); // ← Add this!
                if (keyboardProps.find(key) != keyboardProps.end()) 
                {
                    std::cout << "keyboard key [" << key << "] found" << std::endl;
                    byteTab.push_back(strToByte(keyboardProps[key]));
                } else if (layoutProps.find(key) != layoutProps.end()) 
                {
                    std::cout << "layout key [" << key << "] found" << std::endl;
                    byteTab.push_back(strToByte(layoutProps[key]));
                } else 
                {
                    std::cerr << "Key not found: " << key << std::endl;
                    byteTab.push_back(0x00);
                }
            }
        } else 
        {
            std::cerr << "Char not found: " << str << std::endl;
            byteTab.push_back(0x00);
        }
        return byteTab;
    }

    static uint8_t strToByte(const std::string& str) {
        if (str.substr(0, 2) == "0x") {
            return static_cast<uint8_t>(std::stoi(str.substr(2), nullptr, 16));
        } else {
            return static_cast<uint8_t>(std::stoi(str));
        }
    }
};

std::string encoder::version = "0.0.1";
bool encoder::debug = false;
std::map<std::string, std::string> encoder::keyboardProps;
std::map<std::string, std::string> encoder::layoutProps;

int main() 
{
    encoder::setup();
    encoder::loop();
    return 0;
}
