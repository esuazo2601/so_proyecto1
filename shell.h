
std::vector<std::string> separarPorEspacios(const std::string& input);
int ejecutar(std::string comando);
std::vector<std::string> separarConPipes (const std::string& input);
int hayPipe(const std::string& input);
int ejecutarConPipes(std::string& comandos);
std::vector<std::vector<char*>> parseCommands(const std::string& input);
std::vector<char*> stringVectorToCharVector(const std::vector<std::string>& strings);
int daemon(const std::string & comandos);
bool contieneSubstring(std::string& cadena, std::string subcadena);
std::vector<std::string> lee_Stats();