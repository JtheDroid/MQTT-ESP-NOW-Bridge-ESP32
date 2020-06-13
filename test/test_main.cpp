#include <Arduino.h>
#include <unity.h>
#include "linked_list.h"
#include "mac_topic.cpp"

void test_add()
{
    int amount{10};
    for (size_t i = 0; i < amount; i++)
    {
        uint8_t mac[6]{(uint8_t)i, 0xFF, 0xFF, 0xFF, 0xFF, (uint8_t)(0xFF - i)};
        String s{"topic"};
        s += i;
        add_mac_topic(mac, s.c_str());
        Serial.printf("i: %d, size: %d\n", i, mac_topic_list.size());
        TEST_ASSERT_EQUAL_INT(i + 1, mac_topic_list.size());
    }
    TEST_ASSERT_EQUAL_INT(amount, mac_topic_list.size());
}

void test_remove()
{
    int amount{5};
    for (size_t i = 0; i < amount; i++)
    {
        uint8_t mac[6]{(uint8_t)i, 0xFF, 0xFF, 0xFF, 0xFF, (uint8_t)(0xFF - i)};
        String s{"topic"};
        s += i;
        remove_mac_topic(mac, s.c_str());
        Serial.printf("i: %d, size: %d\n", i, mac_topic_list.size());
        TEST_ASSERT_EQUAL_INT(9 - i, mac_topic_list.size());
    }
    TEST_ASSERT_EQUAL_INT(10 - amount, mac_topic_list.size());
}

void test_clear()
{
    mac_topic_list.clear();
    TEST_ASSERT_EQUAL_INT(0, mac_topic_list.size());
}

void test_linked_list()
{
    linked_list<int> list;
    TEST_ASSERT_EQUAL_UINT16(0, list.size());
    int val = 0;
    list.add_to_start(val);
    TEST_ASSERT_EQUAL_UINT16(1, list.size());
    list.add_to_end(val);
    TEST_ASSERT_EQUAL_UINT16(2, list.size());
    for (size_t i = 0; i < 10; i++)
    {
        int i_ = i;
        list.add_to_start(i_);
        TEST_ASSERT_EQUAL_UINT16(3 + i, list.size());
    }
    list.clear();
    TEST_ASSERT_EQUAL_UINT16(0, list.size());
    list.add_to_end(val);
    list.remove_last();
    TEST_ASSERT_EQUAL_UINT16(0, list.size());
}

void setup()
{
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_linked_list);
    RUN_TEST(test_add);
    RUN_TEST(test_remove);
    RUN_TEST(test_clear);
    delay(1000);
    UNITY_END();
}

void loop()
{
}
