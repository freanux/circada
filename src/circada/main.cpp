#include "Application.hpp"

int main(int argc, char *argv[]) {
    /*
    std::string str("äöü: ");
    for (UTF8Iterator it = str.begin(); it != str.end(); it++) {
        std::cout << *it << std::endl;
        std::cout << it.get_sequence() << std::endl;
    }

    return 0;
    */

    initialize_tls();
    Circada::Configuration config(".circada");
    Application app(config);
    app.run();
    deinitialize_tls();

    return 0;
}
