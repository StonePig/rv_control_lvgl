#include "wifi_utils.h"
#include <string.h>

static wifi_utils_scan_entry_t s_scan_results[WIFI_UTILS_SCAN_MAX];
static int s_scan_count = 0;
static volatile int s_scan_pending = 0;

static volatile int s_connect_result_pending = 0; /* 0 none, 1 success, -1 fail */
static char s_last_success_ssid[WIFI_UTILS_SSID_MAX_LEN]; /* 连接成功时 JNI 传入的 SSID，用于界面立即显示 */

bool set_wifi_enabled(bool enabled)
{
    return wifi_platform_set_enabled(enabled);
}

bool is_wifi_enabled(void)
{
    return wifi_platform_is_enabled();
}

void scan_wifi_networks(void)
{
    wifi_platform_scan();
}

void connect_to_wifi(const char *ssid, const char *password, void *user_data)
{
    (void)user_data;
    wifi_platform_connect(ssid ? ssid : "", password ? password : "");
}

void disconnect_wifi(void)
{
    wifi_platform_disconnect();
}

bool get_connected_ssid(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return false;
    return wifi_platform_get_connected_ssid(buf, buf_size);
}

void handle_wifi_scan_results(const char **ssids, const int *rssis, const int *secured, int count)
{
    if (count < 0) return;
    if (count > WIFI_UTILS_SCAN_MAX) count = WIFI_UTILS_SCAN_MAX;
    s_scan_count = count;
    if (ssids && count > 0) {
        for (int i = 0; i < count; i++) {
            if (ssids[i]) {
                strncpy(s_scan_results[i].ssid, ssids[i], WIFI_UTILS_SSID_MAX_LEN - 1);
                s_scan_results[i].ssid[WIFI_UTILS_SSID_MAX_LEN - 1] = '\0';
            } else {
                s_scan_results[i].ssid[0] = '\0';
            }
            s_scan_results[i].rssi = (rssis && i < count) ? rssis[i] : 0;
            s_scan_results[i].secured = (secured && i < count) ? secured[i] : 0;
        }
    }
    s_scan_pending = 1;
}

void handle_wifi_connect_result(bool success, const char *ssid)
{
    s_connect_result_pending = success ? 1 : -1;
    if (success && ssid && ssid[0] != '\0') {
        strncpy(s_last_success_ssid, ssid, WIFI_UTILS_SSID_MAX_LEN - 1);
        s_last_success_ssid[WIFI_UTILS_SSID_MAX_LEN - 1] = '\0';
    } else {
        s_last_success_ssid[0] = '\0';
    }
}

bool get_display_connected_ssid(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return false;
    if (wifi_platform_get_connected_ssid(buf, buf_size) && buf[0] != '\0')
        return true;
    /* 系统 getConnectionInfo() 可能未更新（如 WifiNetworkSpecifier），用连接成功时保存的 SSID，且不在此清除 */
    if (s_last_success_ssid[0] != '\0') {
        strncpy(buf, s_last_success_ssid, buf_size - 1);
        buf[buf_size - 1] = '\0';
        return true;
    }
    return false;
}

void wifi_utils_clear_last_success_ssid(void)
{
    s_last_success_ssid[0] = '\0';
}

int wifi_utils_consume_scan_results(wifi_utils_scan_entry_t *entries, int max_count)
{
    if (!s_scan_pending) return -1;
    if (!entries || max_count <= 0) { s_scan_pending = 0; return 0; }
    int n = s_scan_count;
    if (n > max_count) n = max_count;
    if (n > 0)
        memcpy(entries, s_scan_results, (size_t)n * sizeof(wifi_utils_scan_entry_t));
    s_scan_pending = 0;
    return n;
}

int wifi_utils_consume_connect_result(void)
{
    int r = s_connect_result_pending;
    if (r == 0) return 0;
    s_connect_result_pending = 0;
    return r;
}

void set_java_vm(void *jvm)
{
    (void)jvm;
}

void set_wifi_utils_class(void *clazz)
{
    (void)clazz;
}
