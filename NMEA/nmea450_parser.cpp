#include "nmea450_decoder.h"
#include <cstring>
#include <sstream>

void Nmea450Decoder::SetOnMsgAssembled(MsgAssembledCallback cb) { m_assembled_cb = cb; }

void Nmea450Decoder::ProcPacket(const uint8_t* payload, size_t len) {
    if (len < 8) return;

    // 1. "UdPbC\0"
    if (std::memcmp(payload, "UdPbC\0", 6) != 0) {
        return; // 
    }

    std::string payload_str(reinterpret_cast<const char*>(payload + 6), len - 6);
    
    // 2. 
    if (!payload_str.empty() && payload_str[0] == '\\') {
        size_t close_tag = payload_str.find('\\', 1);
        if (close_tag != std::string::npos) {
            std::string tag_block = payload_str.substr(1, close_tag - 1);
            std::string nmea_msg = payload_str.substr(close_tag + 1);
            
            // 
            while(!nmea_msg.empty() && (nmea_msg.back() == '\n' || nmea_msg.back() == '\r')) {
                nmea_msg.pop_back();
            }

            HandleTagBlock(tag_block, nmea_msg);
        }
    }
}

void Nmea450Decoder::HandleTagBlock(const std::string& tag_block, const std::string& nmea_msg) {
    size_t star_pos = tag_block.find('*');
    if (star_pos == std::string::npos) return;

    // 
    std::string tags_data = tag_block.substr(0, star_pos);
    std::vector<std::string> tags = SplitStr(tags_data, ',');

    std::string src = "";
    std::string group_tag = "";

    // 
    for (const auto& tag : tags) {
        if (tag.rfind("s:", 0) == 0) {       // (Source)
            src = tag.substr(2);
        } else if (tag.rfind("g:", 0) == 0) { // 
            group_tag = tag.substr(2);
        }
    }

    // 
    if (group_tag.empty()) {
        if (m_assembled_cb) {
            m_assembled_cb(nmea_msg, src);
        }
    } 
    // (AIS 5)
    else {
        // g: [_]-[_]-[id_] (: 1-2-5678)
        std::vector<std::string> g_parts = SplitStr(group_tag, '-');
        if (g_parts.size() != 3) return;

        int cur_line = std::stoi(g_parts[0]);
        int total_lines = std::stoi(g_parts[1]);
        std::string internal_group_id = src + "_" + g_parts[2]; // 

        auto& group = m_nmea_group_pool[internal_group_id];
        group.timestamp = std::chrono::steady_clock::now();
        group.total_lines = total_lines;
        group.lines[cur_line] = nmea_msg;

        // 
        if (group.lines.size() == static_cast<size_t>(group.total_lines)) {
            std::string fully_assembled_msg = "";
            
            for (int i = 1; i <= group.total_lines; ++i) {
                std::string part = group.lines[i];
                if (i == 1) {
                    fully_assembled_msg = part; // (!  $)
                } else {
                    // ('*')
                    size_t first_comma = part.find(',');
                    size_t star = part.find('*');
                    if (first_comma != std::string::npos && star != std::string::npos && star > first_comma) {
                        fully_assembled_msg.insert(fully_assembled_msg.find('*'), part.substr(first_comma, star - first_comma));
                    }
                }
            }
            
            m_nmea_group_pool.erase(internal_group_id); // 

            if (m_assembled_cb) {
                m_assembled_cb(fully_assembled_msg, src);
            }
        }
    }
}

void Nmea450Decoder::CleanupTimeouts() {
    auto now = std::chrono::steady_clock::now();
    // 
    for (auto it = m_nmea_group_pool.begin(); it != m_nmea_group_pool.end();) {
        if (now - it->second.timestamp > std::chrono::seconds(3)) {
            it = m_nmea_group_pool.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<std::string> Nmea450Decoder::SplitStr(const std::string& str, char delimiter) const {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
