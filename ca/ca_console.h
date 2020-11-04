#ifndef _CA_CONSOLE_H_
#define _CA_CONSOLE_H_

#include <string>

typedef enum ConsoleColor
{
    kConsoleColor_Black,
    kConsoleColor_Red,
    kConsoleColor_Green,
    kConsoleColor_Yellow,
    kConsoleColor_Blue,
    kConsoleColor_Purple,
    kConsoleColor_DarkGreen,
    kConsoleColor_White,

} ConsoleColor;

class ca_console
{
public:
    ca_console(const ConsoleColor foregroundColor = kConsoleColor_White, 
                const ConsoleColor backgroundColor = kConsoleColor_Black, 
                bool highlight = false);
    ~ca_console();
    
    const std::string color(); 
    void setColor(const ConsoleColor foregroundColor = kConsoleColor_White, 
                const ConsoleColor backgroundColor = kConsoleColor_Black, 
                bool highlight = false);
    
    const std::string reset(); 
    void clear(); 

    operator const char * ();
    operator char * ();

private:
    std::string bColorString;
    std::string fColorString;
    bool isHightLight;
};

#endif 