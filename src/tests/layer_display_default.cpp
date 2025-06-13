#include "catch2/catch_amalgamated.hpp"
#include "board/board_layers.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>

TEST_CASE("Check that all inner layers exist")
{
    const auto layers = horizon::BoardLayers::get_layers();
    for (unsigned int i = 0; i < horizon::BoardLayers::max_inner_layers; i++) {
        const int layer = -1 - i;
        CHECK(std::find(layers.begin(), layers.end(), layer) != layers.end());
    }
}

TEST_CASE("Check that all board layers are included in defaults")
{
    const auto j = horizon::json_from_resource("/org/horizon-eda/horizon/imp/layer_display_default.json").at("layers");
    const auto layers = horizon::BoardLayers::get_layers();
    for (const auto layer : layers) {
        SECTION(horizon::BoardLayers::get_layer_name(layer))
        CHECK(j.count(std::to_string(layer)));
    }
}
