#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <filesystem>
#include "Arvores.h"
const int ALFABETO = 26; 

using namespace std;
/*
Classe que representa um nó da árvore Trie. Cada nó contém um array de ponteiros para seus filhos (representando as letras do alfabeto) e um booleano que indica se o nó representa o fim de uma palavra.
*/
class NoTrie{
    public:
        NoTrie *filhos[ALFABETO]; // Letras 'a' a 'z'
        bool fimPalavra; // Indica se o nó representa o fim de uma palavra

        NoTrie(){
            fimPalavra = false;// Inicializa como falso
            for (int i = 0; i < ALFABETO;i++){
                filhos[i] = nullptr;
            }
        }
};

class Trie{
    private:
        NoTrie *raiz;

        void liberar(NoTrie *no){
            if(no != nullptr){
                for (int i = 0; i < ALFABETO; i++){
                    liberar(no->filhos[i]);
                }
                delete no;
            }
        }
    
        public:
            Trie(){
                raiz = new NoTrie(); // Inicializa a raiz da Trie. O new tem a função de alocar memória para o nó raiz da Trie, garantindo que ele esteja pronto para armazenar palavras.
            }
            ~Trie(){
                liberar(raiz); // Libera a memória alocada para a Trie chamando a função liberar, que percorre recursivamente todos os nós da Trie e os deleta.
            }
            void inserir(const string &palavra){
                NoTrie *atual = raiz;
                    for(char c : palavra){
                        int indice = c - 'a';
                        if(atual->filhos[indice] == nullptr){
                            atual->filhos[indice] = new NoTrie(); // Cria um novo nó se não existir
                        }
                        atual  = atual->filhos[indice]; // Move para o próximo nó
                    }
                    atual->fimPalavra = true; // Marca o fim da palavra
            }

            bool buscar(const string &palavra){
                NoTrie *atual = raiz;
                for(char c: palavra){
                    int indice = c - 'a';
                    if(atual->filhos[indice] ==nullptr){
                        return false; // caminha não existe -> palavra não está na trie
                    }
                    atual = atual->filhos[indice]; // Move para o próximo nó
                }
                return atual->fimPalavra; // Chega até o fim, mas precisa ser fim de palavra de verdade
            }

            bool possuiPrefixo(const string &prefixo){
                NoTrie *atual = raiz;
                for(char c : prefixo){
                    int indice = c - 'a';
                    if (atual->filhos[indice] == nullptr){
                        return false; // Caminho não existe -> prefixo não está na trie
                    }
                    atual = atual->filhos[indice]; // Move para o próximo nó
                }
                return true; // Chega até o fim, indicando que o prefixo existe na trie. So precisa existir o caminho, não precisa ser fim de palavra
            
            }
        private:
            bool estaVazio(NoTrie *no){
                for (int i = 0; i < ALFABETO; i++){
                    if(no->filhos[i] != nullptr){
                        return false;
                    }
                }
                return true;
            }
            // Retorna true se o nó 'no' pode ser apagado pelo pai que o chamou

            bool removerAux(NoTrie *no, const string &palavra, int profundidade){
                if(no == nullptr){
                    return false; // Palavra não encontrada
                } if (profundidade == (int)palavra.size()){
                    // Nó que representa o fim da palavra a ser removida
                    if(!no->fimPalavra){
                        return false; // Palavra não encontrada
                    }
                    no->fimPalavra = false; // Desmarca o fim da palavra
                    return estaVazio(no); // Retorna true se o nó não tiver filhos, indicando que pode ser apagado
                }
                int indice = palavra[profundidade] - 'a';
                if(removerAux(no->filhos[indice], palavra, profundidade+1)){
                    delete no->filhos[indice];
                    no->filhos[indice] = nullptr;
                    return (!no->fimPalavra) && estaVazio(no); // Retorna true se o nó atual não for fim de palavra e não tiver filhos}
                }
                return false;
            }
        public:
            void remover(const string &palavra){
                removerAux(raiz, palavra, 0); // Inicia a remoção a partir da raiz da Trie
            }

        private:
            // Percorre a trie em profundidade reconstruindo cada palavra
            // letra a letra (usado pela opcao de exportar para .txt)
            void coletarPalavras(NoTrie *no, string atual, vector<string> &saida){
                if(no == nullptr) return;
                if(no->fimPalavra) saida.push_back(atual);
                for(int i = 0; i < ALFABETO; i++){
                    if(no->filhos[i] != nullptr){
                        coletarPalavras(no->filhos[i], atual + char('a' + i), saida);
                    }
                }
            }

            // gera as linhas do arquivo .dot. Diferente de Splay/Treap, o
            // "rotulo" de cada aresta e' a LETRA (indice do array filhos[]),
            // nao um valor guardado no no -- e isso que evidencia visualmente
            // o compartilhamento de prefixos, que e' a ideia central da Trie.
            // Nos que fecham uma palavra (fimPalavra) saem com contorno duplo.
            int exportarDot(NoTrie *no, ofstream &arquivo, int &contador) {
                int idAtual = contador++;
                if (no->fimPalavra) {
                    arquivo << "    n" << idAtual << " [label=\"\", shape=doublecircle, fillcolor=\"#ffd699\"];\n";
                } else {
                    arquivo << "    n" << idAtual << " [label=\"\"];\n";
                }
                for (int i = 0; i < ALFABETO; i++) {
                    if (no->filhos[i] != nullptr) {
                        int idFilho = exportarDot(no->filhos[i], arquivo, contador);
                        arquivo << "    n" << idAtual << " -> n" << idFilho
                                << " [label=\"" << char('a' + i) << "\"];\n";
                    }
                }
                return idAtual;
            }
        public:
            vector<string> listarPalavras(){
                vector<string> saida;
                coletarPalavras(raiz, "", saida);
                return saida;
            }

            // Exporta a trie atual para um arquivo .dot (formato Graphviz),
            // rotulado com "titulo". Renderize com:
            //   dot -Tpng caminho.dot -o caminho.png
            void exportarDot(const string &caminho, const string &titulo) {
                filesystem::path caminhoPath(caminho);
                if (caminhoPath.has_parent_path()) {
                    filesystem::create_directories(caminhoPath.parent_path());
                }

                ofstream arquivo(caminho);
                if (!arquivo.is_open()) {
                    cerr << "Aviso: nao foi possivel criar \"" << caminho << "\".\n";
                    return;
                }
                arquivo << "// Trie -- gerado automaticamente\n";
                arquivo << "digraph TrieTree {\n";
                arquivo << "    label=\"" << titulo
                        << "\\n(circulo duplo laranja = fim de palavra)\";\n";
                arquivo << "    labelloc=t;\n";
                arquivo << "    node [shape=circle, style=filled, fillcolor=\"#e0e0e0\", fixedsize=true, width=0.4];\n";
                int contador = 0;
                exportarDot(raiz, arquivo, contador);
                arquivo << "}\n";
                cout << "Trie exportada para \"" << caminho << "\" (renderize com: dot -Tpng "
                     << caminho << " -o " << caminho.substr(0, caminho.find_last_of('.')) << ".png)\n";
            }
};

int main(int argc, char* argv[]){
    Trie trie;
    using Relogio = chrono::high_resolution_clock;
    const char* ARQUIVO_RESULTADOS = "resultados/resultados.csv";

    // "input/dados_trie_patricia.dat" e relativo ao diretorio de onde o
    // executavel e rodado -- execute a partir da pasta EmCpp. Aceita
    // opcionalmente o caminho do arquivo como argumento (mesma convencao
    // usada em Splay/Treap/KDTree), pra rodar com outros tamanhos sem
    // recompilar: ./build/Trie input/gerados/dados_trie_patricia_10000.dat
    string caminhoDados = (argc > 1) ? argv[1] : "input/dados_trie_patricia.dat";
    vector<string> palavrasIniciais = lerPalavras(caminhoDados);

    auto inicioInsercao = Relogio::now();
    for (const string &p : palavrasIniciais) trie.inserir(p);
    auto fimInsercao = Relogio::now();
    double tempoInsercaoMs = chrono::duration<double, milli>(fimInsercao - inicioInsercao).count();
    cout << palavrasIniciais.size() << " palavras carregadas do arquivo de entrada.\n";
    cout << "  tempo de insercao: " << tempoInsercaoMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "Trie", "palavras", "insercao", (long long)palavrasIniciais.size(), tempoInsercaoMs);

    // busca TODAS as palavras carregadas, de uma vez, so pra medir o tempo
    // (o menu interativo abaixo continua igual, pra buscas avulsas manuais)
    auto inicioBusca = Relogio::now();
    int encontrados = 0;
    for (const string &p : palavrasIniciais) if (trie.buscar(p)) encontrados++;
    auto fimBusca = Relogio::now();
    double tempoBuscaMs = chrono::duration<double, milli>(fimBusca - inicioBusca).count();
    cout << "  tempo de busca (" << palavrasIniciais.size() << " buscas, " << encontrados << " encontradas): " << tempoBuscaMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "Trie", "palavras", "busca", (long long)palavrasIniciais.size(), tempoBuscaMs);

    // remove metade das palavras carregadas, so pra medir tempo de remocao
    size_t metade = palavrasIniciais.size() / 2;
    auto inicioRemocao = Relogio::now();
    for (size_t i = 0; i < metade; i++) trie.remover(palavrasIniciais[i]);
    auto fimRemocao = Relogio::now();
    double tempoRemocaoMs = chrono::duration<double, milli>(fimRemocao - inicioRemocao).count();
    cout << "  tempo de remocao (" << metade << " palavras): " << tempoRemocaoMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "Trie", "palavras", "remocao", (long long)metade, tempoRemocaoMs);

    // reinsere as palavras removidas no benchmark acima, pra o menu
    // interativo abaixo comecar com a trie completa de novo (nao da pra
    // usar "trie = Trie()" aqui pq a classe nao tem operator= proprio --
    // isso deixaria um ponteiro pendurado)
    for (size_t i = 0; i < metade; i++) trie.inserir(palavrasIniciais[i]);

    // ---------- Demonstracao automatica (sem menu interativo) ----------
    // Antes, essa parte era um menu (cin >> opsao) que dependia de digitacao
    // manual. Agora roda tudo sozinho, igual as outras 4 estruturas (Splay,
    // Treap, KDTree, Patricia): insere um conjunto pequeno e fixo de
    // palavras com prefixos compartilhados, demonstra busca, busca por
    // prefixo e remocao, e exporta o resultado final pra um .txt.
    cout << "\n\n===== Demonstracao automatica (exemplo pequeno p/ Secao 3) =====\n";
    Trie exemplo;
    vector<string> palavrasExemplo = {
        "casa", "carro", "carroca", "cachorro", "caderno",
        "dado", "dedo", "dedoista", "dedal", "casaco"
    };
    for (const string &p : palavrasExemplo) exemplo.inserir(p);

    cout << "\nPalavras inseridas: ";
    for (const string &p : palavrasExemplo) cout << p << " ";
    cout << "\n";

    cout << "\nBusca por \"carro\": "
         << (exemplo.buscar("carro") ? "encontrada" : "nao encontrada") << "\n";
    cout << "Busca por \"carruagem\" (nunca inserida): "
         << (exemplo.buscar("carruagem") ? "encontrada" : "nao encontrada") << "\n";
    cout << "Existe o prefixo \"cas\"? "
         << (exemplo.possuiPrefixo("cas") ? "sim" : "nao") << "\n";
    cout << "Existe o prefixo \"zzz\"? "
         << (exemplo.possuiPrefixo("zzz") ? "sim" : "nao") << "\n";

    cout << "\nRemovendo \"carro\" (mas \"carroca\" compartilha o prefixo e deve continuar existindo)...\n";
    exemplo.exportarDot("resultados/diagramas/trie_antes.dot", "Trie - ANTES de remover \\\"carro\\\"");
    exemplo.remover("carro");
    exemplo.exportarDot("resultados/diagramas/trie_depois.dot", "Trie - DEPOIS de remover \\\"carro\\\"");
    cout << "Busca por \"carro\" apos remocao: "
         << (exemplo.buscar("carro") ? "encontrada" : "nao encontrada") << "\n";
    cout << "Busca por \"carroca\" apos a remocao de \"carro\": "
         << (exemplo.buscar("carroca") ? "encontrada" : "nao encontrada") << "\n";

    // exporta a lista final de palavras da trie de exemplo (substitui a
    // antiga opcao 5 do menu, que exigia digitar o nome do arquivo na mao)
    const string arquivoExportado = "resultados/trie_exemplo.txt";
    ofstream arquivoSaida(arquivoExportado);
    if (arquivoSaida.is_open()) {
        vector<string> palavrasFinais = exemplo.listarPalavras();
        arquivoSaida << "===== Palavras armazenadas na Trie de exemplo (" << palavrasFinais.size() << ") =====\n\n";
        for (const string &p : palavrasFinais) arquivoSaida << p << "\n";
        cout << "\nLista de palavras exportada para \"" << arquivoExportado << "\".\n";
    } else {
        cerr << "Aviso: nao foi possivel criar \"" << arquivoExportado << "\".\n";
    }

    return 0;
}