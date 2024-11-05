
//  mgream 2024

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <Arduino.h>

#include <WiFi.h>

// -----------------------------------------------------------------------------------------------

template<size_t N>
String BytesToHexString(const uint8_t bytes[], const char *separator = ":") {
    constexpr size_t separator_max = 1;    // change if needed
    if (strlen(separator) > separator_max)
        return String("");
    char buffer[(N * 2) + ((N - 1) * separator_max) + 1] = { '\0' }, *buffer_ptr = buffer;
    for (auto i = 0; i < N; i++) {
        if (i > 0 && separator[0] != '\0')
            for(auto separator_ptr = separator; *separator_ptr != '\0';)
                *buffer_ptr++ = *separator_ptr++;
        static const char hex_chars[] = "0123456789abcdef";
        *buffer_ptr++ = hex_chars[(bytes[i] >> 4) & 0x0F];
        *buffer_ptr++ = hex_chars[bytes[i] & 0x0F];
    }
    *buffer_ptr = '\0';
    return String(buffer);
}
template<typename T>
String ArithmeticToString(const T &n, const int x = -1, const bool t = false) {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
    char s[64 + 1];
    if constexpr (std::is_integral_v<T>)
        return (x == -1 || x == 10) ? String(n) : String(ltoa(static_cast<long>(n), s, x));
    else if constexpr (std::is_floating_point_v<T>) {
        dtostrf(n, 0, x == -1 ? 2 : x, s);
        if (t) {
            char *d = nullptr, *e = s;
            while (*e != '\0') {
                if (*e == '.')
                    d = e;
                e++;
            }
            e--;
            if (d)
                while (e > d + 1 && *e == '0') *e-- = '\0';
            else
                *e++ = '.', *e++ = '0', *e = '\0';
        }
        return String(s);
    }
};

// -----------------------------------------------------------------------------------------------

static String _ssid_to_string(const uint8_t ssid[], const uint8_t ssid_len) {
    return String(reinterpret_cast<const char *>(ssid), ssid_len);
}
static String _bssid_to_string(const uint8_t bssid[]) {
    return BytesToHexString<6>(bssid);
}
static String _authmode_to_string(const wifi_auth_mode_t authmode) {
    switch (authmode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2-PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "ENTERPRISE";
        default: return "UNDEFINED_(" + ArithmeticToString(static_cast<int>(authmode)) + ")";
    }
}
static String _error_to_string(const wifi_err_reason_t reason) {
    switch (reason) {
        case WIFI_REASON_UNSPECIFIED: return "UNSPECIFIED";
        case WIFI_REASON_AUTH_EXPIRE: return "AUTH_EXPIRE";
        case WIFI_REASON_AUTH_LEAVE: return "AUTH_LEAVE";
        case WIFI_REASON_ASSOC_EXPIRE: return "ASSOC_EXPIRE";
        case WIFI_REASON_ASSOC_TOOMANY: return "ASSOC_TOOMANY";
        case WIFI_REASON_NOT_AUTHED: return "NOT_AUTHED";
        case WIFI_REASON_NOT_ASSOCED: return "NOT_ASSOCED";
        case WIFI_REASON_ASSOC_LEAVE: return "ASSOC_LEAVE";
        case WIFI_REASON_ASSOC_NOT_AUTHED: return "ASSOC_NOT_AUTHED";
        case WIFI_REASON_DISASSOC_PWRCAP_BAD: return "DISASSOC_PWRCAP_BAD";
        case WIFI_REASON_DISASSOC_SUPCHAN_BAD: return "DISASSOC_SUPCHAN_BAD";
        case WIFI_REASON_IE_INVALID: return "IE_INVALID";
        case WIFI_REASON_MIC_FAILURE: return "MIC_FAILURE";
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4WAY_HANDSHAKE_TIMEOUT";
        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: return "GROUP_KEY_UPDATE_TIMEOUT";
        case WIFI_REASON_IE_IN_4WAY_DIFFERS: return "IE_IN_4WAY_DIFFERS";
        case WIFI_REASON_GROUP_CIPHER_INVALID: return "GROUP_CIPHER_INVALID";
        case WIFI_REASON_PAIRWISE_CIPHER_INVALID: return "PAIRWISE_CIPHER_INVALID";
        case WIFI_REASON_AKMP_INVALID: return "AKMP_INVALID";
        case WIFI_REASON_UNSUPP_RSN_IE_VERSION: return "UNSUPP_RSN_IE_VERSION";
        case WIFI_REASON_INVALID_RSN_IE_CAP: return "INVALID_RSN_IE_CAP";
        case WIFI_REASON_802_1X_AUTH_FAILED: return "802_1X_AUTH_FAILED";
        case WIFI_REASON_CIPHER_SUITE_REJECTED: return "CIPHER_SUITE_REJECTED";
        case WIFI_REASON_BEACON_TIMEOUT: return "BEACON_TIMEOUT";
        case WIFI_REASON_NO_AP_FOUND: return "NO_AP_FOUND";
        case WIFI_REASON_AUTH_FAIL: return "AUTH_FAIL";
        case WIFI_REASON_ASSOC_FAIL: return "ASSOC_FAIL";
        case WIFI_REASON_HANDSHAKE_TIMEOUT: return "HANDSHAKE_TIMEOUT";
        case WIFI_REASON_CONNECTION_FAIL: return "CONNECTION_FAIL";
        default: return "UNDEFINED_(" + ArithmeticToString(static_cast<int>(reason)) + ")";
    }
}

// -----------------------------------------------------------------------------------------------

static volatile bool wiFiConnected = false, wiFiAllocated = false;
static void wiFiEvents(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        Serial.printf("WiFiEvent: ARDUINO_EVENT_WIFI_STA_CONNECTED, ssid=%s, bssid=%s, channel=%d, authmode=%s, rssi=%d\n",
                      _ssid_to_string(info.wifi_sta_connected.ssid, info.wifi_sta_connected.ssid_len).c_str(),
                      _bssid_to_string(info.wifi_sta_connected.bssid).c_str(), (int)info.wifi_sta_connected.channel,
                      _authmode_to_string(info.wifi_sta_connected.authmode).c_str(), WiFi.RSSI());
        wiFiConnected = true;
    } else if (event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP) {
        Serial.printf("WiFiEvent: ARDUINO_EVENT_WIFI_STA_GOT_IP, address=%s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
        wiFiAllocated = true;
    } else if (event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        Serial.printf("WiFiEvent: ARDUINO_EVENT_WIFI_STA_DISCONNECTED, ssid=%s, bssid=%s, reason=%s\n",
                      _ssid_to_string(info.wifi_sta_disconnected.ssid, info.wifi_sta_disconnected.ssid_len).c_str(),
                      _bssid_to_string(info.wifi_sta_disconnected.bssid).c_str(), _error_to_string((wifi_err_reason_t)info.wifi_sta_disconnected.reason).c_str());
        wiFiConnected = false;
        wiFiAllocated = false;
    }
}

#include "Secrets.hpp"
//#define WIFI_SSID "ssid"    // Secrets.hpp
//#define WIFI_PASS "pass"    // Secrest.hpp
#define WIFI_HOST "mdnstester"

void wiFiWaitfor(const char* name, volatile bool& flag) {
    Serial.printf("WiFi waiting for %s ", name);
    while (!flag) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf(" done\n");
}

void wiFiConnect() {

    WiFi.persistent(false);
    WiFi.onEvent(wiFiEvents);
    WiFi.setHostname(WIFI_HOST);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    wiFiWaitfor ("connect", wiFiConnected);
    wiFiWaitfor ("allocation", wiFiAllocated);
}

// -----------------------------------------------------------------------------------------------

#include <WiFiUdp.h>
#include "ArduinoLightMDNS.hpp"

WiFiUDP *udp;
MDNS *mdns;

void setup() {

    Serial.begin(115200);
    delay(5 * 1000);

    Serial.printf("WiFi startup\n");
    wiFiConnect();

    Serial.printf("MDNS startup\n");
    udp = new WiFiUDP();
    mdns = new MDNS(*udp);
    auto status = mdns->begin();
    if (status != MDNS::Success)
        Serial.printf("MDNS begin: error=%d\n", status);
    mdns->start(WiFi.localIP(), WIFI_HOST);
    mdns->addServiceRecord(MDNS::ServiceTCP, 80, "webserver._http", { "type=example", "not_really=true" });
}

void loop() {
    Serial.printf ("loop\n");
    auto status = mdns->process();
    if (status != MDNS::Success)
        Serial.printf("MDNS process: error=%d\n", status);
    delay (500);
}

// -----------------------------------------------------------------------------------------------
