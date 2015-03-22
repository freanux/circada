#include "Application.hpp"

int main(int argc, char *argv[]) {
    initialize_tls();
    Circada::Configuration config(".circada");
    Application app(config);
    app.run();
    deinitialize_tls();

    return 0;
}
