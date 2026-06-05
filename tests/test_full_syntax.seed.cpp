#include "seed_lib.hpp"

struct EmptyType;
struct Tarefa;

struct EmptyType {

};

struct Tarefa {
int id;
std::string titulo;
bool concluida;

    static Tarefa nova(int id, std::string titulo);
    void marcar();
    std::string formatar();
};

Tarefa Tarefa::nova(int id, std::string titulo) {
auto t = Tarefa ( ) ;
t . id = id ;
t . titulo = titulo ;
t . concluida = false ;
return t ;
}

void Tarefa::marcar() {
auto& self = *this;
self . concluida = true ;
}

std::string Tarefa::formatar() {
auto& self = *this;
const auto status = ((self . concluida ) ? (std::string ( "✓" ) ) : (std::string ( "○" ) )) ;
return std::string ( "[" ) + status + "] " + self . titulo ;
}


int main(int argc, char** argv) {
std::vector<Tarefa> tarefas = {} ;
auto t1 = Tarefa :: nova ( 1 , "Aprender SEED" ) ;
auto t2 = Tarefa :: nova ( 2 , "Criar um projeto" ) ;
const auto m = match empty_match { } ;
t1 . marcar ( ) ;
tarefas . push_back ( t1 ) ;
tarefas . push_back ( t2 ) ;
seed::println ( "=== Lista de Tarefas ===" ) ;
for (auto& tarefa : tarefas ) {
seed::println ( tarefa . formatar ( ) ) ;
}
;
seed::println ( "Test OK!" ) ;
}

