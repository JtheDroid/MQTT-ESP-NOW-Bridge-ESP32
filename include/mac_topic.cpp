#ifndef MAC_TOPIC_CPP
#define MAC_TOPIC_CPP
#include <Arduino.h>
#include <esp_now.h>
#include <functional>
#include "linked_list.h"

typedef struct mac_topic
{
  uint8_t *mac;
  char *topic;
} mac_topic;

linked_list<mac_topic> mac_topic_list;

bool mac_equals(const uint8_t *mac1, const uint8_t *mac2)
{
  for (size_t i = 0; i < ESP_NOW_ETH_ALEN; i++)
    if (mac1[i] != mac2[i])
      return false;
  return true;
}

// bool mac_topic_equals(const mac_topic *mc1, const mac_topic *mc2)
// {
//   return mac_equals(mc1->mac, mc2->mac) && strcmp(mc1->topic, mc2->topic) == 0;
// }

void add_mac_topic(const uint8_t *mac, const char *topic)
{
  if (mac_topic_list.true_for_any([mac, topic](mac_topic *mc) {
        return mac_equals(mac, mc->mac) && strcmp(topic, mc->topic) == 0;
      }))
    return;
  Serial.println("adding to list");
  mac_topic *mc = new mac_topic{
      new uint8_t[6],
      new char[strlen(topic)]};
  memcpy(mc->mac, mac, sizeof(uint8_t) * 6);
  strcpy(mc->topic, topic);
  mac_topic_list.add_to_end(mc);
}

void remove_mac_topic(const uint8_t *mac, const char *topic)
{
  mac_topic_list.remove_if_true([mac, topic](mac_topic *mc) {
    if (mac_equals(mac, mc->mac) && strcmp(topic, mc->topic) == 0)
    {
      //delete[] mc->mac;
      //delete[] mc->topic;
      delete mc;
      return true;
    }
    return false;
  },
                                true);
}

void execute_for_topic(const char *topic, std::function<void(const uint8_t *mac)> execute)
{
  mac_topic_list.execute_for_all([topic, execute](mac_topic *mc) {
    if (strcmp(topic, mc->topic) == 0 || strcmp("#", mc->topic) == 0)
      execute(mc->mac);
  });
}
#endif