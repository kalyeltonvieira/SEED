#include "seed_lib.hpp"




int main(int argc, char** argv) {
seed::println ( std::string("=== Calculadora SEED ===") ) ;
seed::println ( std::string("Operações disponíveis: +, -, *, /") ) ;
seed::println ( std::string("Digite 'sair' para encerrar") ) ;
seed::println ( std::string("") ) ;
while (true) {
const auto input = prompt ( std::string("Expressão (ex: 5 + 3): ") ) ;
if (input == std::string("sair") ) {
seed::println ( std::string("Até logo!") ) ;
break ;
}
;
const auto parts = input seed::split ( std::string(" ") ) ;
if (parts.size() != 3 ) {
seed::println ( std::string("Erro: formato inválido. Use: número operador número") ) ;
continue ;
}
;
const auto a = parts [ 0 ] seed::to_float ( ) ;
const auto op = parts [ 1 ] ;
const auto b = parts [ 2 ] seed::to_float ( ) ;
auto __seed_try_result_0 = match op { std::string("+") = > a + b std::string("-") = > a - b std::string("*") = > a * b std::string("/") = > ((b == 0 )  (std::string("Erro: divisão por zero") ) : (a / b )) [&](auto _) { return std::string("Erro: operador inválido") ; } ; seed::println ( std::string("Resultado: ") + std::to_string(result) ) ; seed::println ( std::string("") ) ; } ; } ; prompt ( msg : std::string ) -> std::string { seed::print ( msg ) ; return read_line ( ) ; } ; ; };
const auto result = *__seed_try_result_0;
}
}

