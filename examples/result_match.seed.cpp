#include "seed_lib.hpp"

struct Config;

struct Config {
std::string name;
int port;

};

auto parse_config(std::string body) -> std::expected<Config, std::string>;
auto load_config(std::string path) -> std::expected<Config, std::string>;

auto parse_config(std::string body) -> std::expected<Config, std::string> {
if (body.size() == 0 ) {
return std::unexpected(std::string(std::string("config vazia") ) ) ;
}
;
return Config{std::string("seed") , 7331 }  ;
}

auto load_config(std::string path) -> std::expected<Config, std::string> {
auto __seed_try_body_0 = seed::fs::read_text(path );
if (!__seed_try_body_0) return std::unexpected(__seed_try_body_0.error());
const auto body = *__seed_try_body_0;
auto __seed_try_config_1 = parse_config ( body );
if (!__seed_try_config_1) return std::unexpected(__seed_try_config_1.error());
const auto config = *__seed_try_config_1;
return config  ;
}

int main(int argc, char** argv) {
{
auto __seed_match_2 = load_config ( std::string("seed.toml") );
if (__seed_match_2) {
auto config = *__seed_match_2;
{
seed::print ( std::string("porta: ") + std::to_string ( config . port ) + std::string("\n") ) ;
return 0 ;
}
}
else {
auto e = __seed_match_2.error();
{
seed::print ( std::string("erro: ") + e+ std::string("\n") ) ;
return 1 ;
}
}
}
;
}

