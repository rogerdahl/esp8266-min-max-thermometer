#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <protocol_examples_common.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <rom/ets_sys.h>
#include <esp_http_server.h>


#include "user_config.h"
#include "ds18b20.h"
#include "ntp.h"
#include "temperature_tracker.h"

os_timer_t read_timer;


static httpd_handle_t server = NULL;
extern DS18B20_Sensors sensors;


esp_err_t get_temperature_handler(httpd_req_t *req);

static void disconnect_handler(void* arg, esp_event_base_t event_base,
    s32 event_id, void* event_data);

static void connect_handler(void* arg, esp_event_base_t event_base,
    s32 event_id, void* event_data);

httpd_handle_t start_webserver();


const u32 MAX_TEMPERATURE_LINE_LENGTH = 256;


void service_init()
{
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Connect to network using SSID and key specified in
  // idf.py menuconfig > Example Connection Information
  ESP_ERROR_CHECK(example_connect());

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

  server = start_webserver();
}



httpd_uri_t temperature = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = get_temperature_handler,
};

httpd_handle_t start_webserver() {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Start the httpd server
  INFO("Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    INFO("Registering URI handlers");
    httpd_register_uri_handler(server, &temperature);
    return server;
  }

  INFO("Error starting server!");
  return NULL;
}

void stop_webserver(httpd_handle_t server)
{
  // Stop the httpd server
  httpd_stop(server);
}

/* An HTTP GET handler */
esp_err_t get_temperature_handler(httpd_req_t *req) {
  char*  buf;
  size_t buf_len;

  /* Get header value string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    /* Copy null terminated value string into buffer */
    if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
      INFO("Found header => Host: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
      INFO("Found header => Test-Header-2: %s", buf);
    }
    free(buf);
  }

  buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
      INFO("Found header => Test-Header-1: %s", buf);
    }
    free(buf);
  }

  httpd_resp_set_type(req, "text/json");

  s8 lineBuf[MAX_TEMPERATURE_LINE_LENGTH + 2];

  // Return tracked temps as JSON
  httpd_resp_send_chunk(req, "[\n", 2);
  for (size_t i = 0; i < getMinMaxCount(); ++i) {
    getMinMaxLine(lineBuf, MAX_TEMPERATURE_LINE_LENGTH, i);
    size_t lineLen = strlen(lineBuf);
    if (i < getMinMaxCount() - 1) {
      strcpy(lineBuf + lineLen, ",\n");
    }
    httpd_resp_send_chunk(req, lineBuf, strlen(lineBuf));
  }
  httpd_resp_send_chunk(req, "\n]\n", 3);
  httpd_resp_send_chunk(req, lineBuf, 0);

  return ESP_OK;
}
void disconnect_handler(void *arg, esp_event_base_t event_base,
                        s32 event_id, void *event_data) {
  httpd_handle_t* server = (httpd_handle_t*) arg;
  if (*server) {
    INFO("Stopping webserver");
    stop_webserver(*server);
    *server = NULL;
  }
}
void connect_handler(void *arg, esp_event_base_t event_base, s32 event_id,
                     void *event_data) {
  httpd_handle_t* server = (httpd_handle_t*) arg;
  if (*server == NULL) {
    INFO("Starting webserver");
    *server = start_webserver();
  }
}
