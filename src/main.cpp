// This main.cpp is for demonstration purposes only
#include <iostream>
#include "serial.hpp"




int main() {
    Serial s;
    s.connect_serial();
    string swag = "Troppo swag";
    while (s.get_available_data() == 0);
    string l = s.get_line();
    cout << l;
    s.send_line(swag);
    while (s.get_available_data() == 0);
    l = s.get_line();
    cout << l;

    return 0;    
}