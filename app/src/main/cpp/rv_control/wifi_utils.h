#ifndef RV_CONTROL_WIFI_UTILS_H
#define RV_CONTROL_WIFI_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_UTILS_SSID_MAX_LEN 64
#define WIFI_UTILS_SCAN_MAX     64

typedef struct {
    char ssid[WIFI_UTILS_SSID_MAX_LEN];
    int  rssi;    /* 信号强度，通常 -100 ~ -30 */
    int  secured; /* 1=需要密码(WPA/WEP等)，0=开放 */
} wifi_utils_scan_entry_t;

/* 平台层由 JNI 实现（lvgl-entrance.cpp） */
bool wifi_platform_set_enabled(bool enabled);
bool wifi_platform_is_enabled(void);
void wifi_platform_scan(void);
void wifi_platform_connect(const char *ssid, const char *password);
void wifi_platform_disconnect(void);
bool wifi_platform_get_connected_ssid(char *buf, size_t buf_size);

/* 供 UI 调用的 API */
bool set_wifi_enabled(bool enabled);
bool is_wifi_enabled(void);
void scan_wifi_networks(void);
void connect_to_wifi(const char *ssid, const char *password, void *user_data);
void disconnect_wifi(void);
bool get_connected_ssid(char *buf, size_t buf_size);

/* 由 JNI 回调（扫描/连接结果），在 Java 线程调用，仅写内部缓冲。secured 可为 NULL 表示全部开放 */
void handle_wifi_scan_results(const char **ssids, const int *rssis, const int *secured, int count);
void handle_wifi_connect_result(bool success, const char *ssid);
/* 取“当前应显示的已连接 SSID”：优先系统返回值，成功连接后系统未更新时用回调传入的 ssid */
bool get_display_connected_ssid(char *buf, size_t buf_size);
/* 用户断开时调用，清掉“上次连接成功”的 SSID 缓存，避免断开后仍显示 */
void wifi_utils_clear_last_success_ssid(void);

/* 在 LVGL 主线程中调用：取走待处理的扫描结果。
 * 返回值：<0 无新数据，0 新扫描结果为空，>0 新扫描结果条目数 */
int wifi_utils_consume_scan_results(wifi_utils_scan_entry_t *entries, int max_count);
/* 返回：0 无待处理，1 连接成功，-1 连接失败 */
int wifi_utils_consume_connect_result(void);

void set_java_vm(void *jvm);
void set_wifi_utils_class(void *clazz);

#ifdef __cplusplus
}
#endif

#endif
