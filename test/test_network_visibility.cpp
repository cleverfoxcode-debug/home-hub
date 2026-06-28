#include <unity.h>
#include <vector>
#include <string>

#include "src/core/utils/NetworkVisibility.h"

void test_visible_when_saved_ssid_is_present(void) {
    const std::vector<std::string> visibleNetworks = {"Guest", "HomeWiFi", "IoT"};
    TEST_ASSERT_TRUE(hub::core::utils::isHomeNetworkVisible("HomeWiFi", visibleNetworks));
}

void test_hidden_when_saved_ssid_is_missing(void) {
    const std::vector<std::string> visibleNetworks = {"Guest", "Other"};
    TEST_ASSERT_FALSE(hub::core::utils::isHomeNetworkVisible("HomeWiFi", visibleNetworks));
}

void test_empty_saved_ssid_is_not_visible(void) {
    const std::vector<std::string> visibleNetworks = {"HomeWiFi"};
    TEST_ASSERT_FALSE(hub::core::utils::isHomeNetworkVisible("", visibleNetworks));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_visible_when_saved_ssid_is_present);
    RUN_TEST(test_hidden_when_saved_ssid_is_missing);
    RUN_TEST(test_empty_saved_ssid_is_not_visible);
    return UNITY_END();
}
