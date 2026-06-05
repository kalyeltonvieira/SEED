#include "seed_lib.hpp"



int main(int argc, char** argv) {
const auto secret = seed::rand::int_range ( 1 , 100 ) ;
int attempts = 0 ;
seed::println ( std::string("=== Guess the Number ===") ) ;
while (true) {
seed::print ( std::string("Enter your guess (1-100): ") ) ;
const auto guess_str = seed::read_line ( ) ;
{
auto __seed_match_0 = seed::parse<int>(seed::trim(guess_str));
if (__seed_match_0) {
auto guess = *__seed_match_0;
{
attempts = attempts + 1 ;
if (guess < secret ) {
seed::println ( std::string("Too low!") ) ;
} else if (guess > secret ) {
seed::println ( std::string("Too high!") ) ;
} else {
seed::println ( std::string("Correct! You guessed in ") + std::to_string(attempts) + std::string(" attempts.") ) ;
break ;
}
;
}
}
else {
{
seed::println ( std::string("Please enter a valid number.") ) ;
}
}
}
;
}
;
return 0 ;
}

