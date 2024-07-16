#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"
#include "os.h"


namespace OS {

// ---------------------------------------
Arch::Terminal *terminalTeste;
Arch::Cpu *cpuTeste;

static std::string bufferedChar;

void boot (Arch::Terminal *terminal, Arch::Cpu *cpu)
{	
	terminalTeste = terminal;
	cpuTeste = cpu;
	terminal->println(Arch::Terminal::Type::Command, "Type commands here");
	terminal->println(Arch::Terminal::Type::App, "Apps output here");
	terminal->println(Arch::Terminal::Type::Kernel, "Kernel output here");
}

// ---------------------------------------

void interrupt(const Arch::InterruptCode interrupt) {
    if (interrupt == Arch::InterruptCode::Keyboard) {
        int typed_char = terminalTeste->read_typed_char();
        char char_typed = static_cast<char>(typed_char);

        if (terminalTeste->is_backspace(typed_char)) {
            if (!bufferedChar.empty()) {
            
                bufferedChar.pop_back();

                terminalTeste->print(Arch::Terminal::Type::Command, '\b'); 
                terminalTeste->print(Arch::Terminal::Type::Command, ' '); 
                terminalTeste->print(Arch::Terminal::Type::Command, '\b'); 
            }
            terminalTeste->println(Arch::Terminal::Type::Kernel, "apagar");
        }

        else if (terminalTeste->is_alpha(typed_char) || terminalTeste->is_num(typed_char)) {
 
            bufferedChar += char_typed;

            terminalTeste->print(Arch::Terminal::Type::Command, char_typed);
        }

        else if (terminalTeste->is_return(typed_char)) {

            for (size_t i = 0; i < bufferedChar.size(); ++i) {
                terminalTeste->print(Arch::Terminal::Type::Command, '\b'); 
                terminalTeste->print(Arch::Terminal::Type::Command, ' ');  
                terminalTeste->print(Arch::Terminal::Type::Command, '\b'); 
            }

            terminalTeste->println(Arch::Terminal::Type::Kernel, "enter");
            terminalTeste->print(Arch::Terminal::Type::App, bufferedChar);
            terminalTeste->println(Arch::Terminal::Type::App, "");
            bufferedChar.clear();
        }
    }
}


// ---------------------------------------

void syscall ()
{

}

// ---------------------------------------

} // end namespace OS
