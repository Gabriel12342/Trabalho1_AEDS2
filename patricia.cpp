#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include "Arvores.h"
using namespace std;

class NoPatricia {
public:
    string rotulo;   // trecho de string representado pela aresta até este nó
    bool fimDePalavra;
    map<char, NoPatricia*> filhos; // indexado pela 1a letra do rótulo do filho

    NoPatricia(const string &r, bool fim) : rotulo(r), fimDePalavra(fim) {}
};

class Patricia {
private:
    NoPatricia *raiz;

    void liberar(NoPatricia *no) {
        for (auto &par : no->filhos) {
            liberar(par.second);
        }
        delete no;
    }

    // Maior prefixo comum entre duas strings
    string prefixoComum(const string &a, const string &b) {
        size_t i = 0;
        size_t max = std::min(a.size(), b.size());
        while (i < max && a[i] == b[i]) {
            i++;
        }
        return a.substr(0, i);
    }

    // Retorna o nó que deve ficar no lugar de 'no': o próprio 'no' (sem mudança
    // estrutural), um nó fundido (após um merge), ou nullptr (se 'no' foi apagado)
    NoPatricia* removerAux(NoPatricia *no, const string &restante) {
        if (restante.empty()) {
            if (!no->fimDePalavra) {
                return no; // a palavra não estava inserida, nada a fazer
            }
            no->fimDePalavra = false;
        } else {
            char c = restante[0];
            auto it = no->filhos.find(c);
            if (it == no->filhos.end()) {
                return no; // caminho não existe, palavra não estava inserida
            }

            NoPatricia *filho = it->second;
            if (restante.compare(0, filho->rotulo.size(), filho->rotulo) != 0) {
                return no; // não bate com o rótulo, palavra não estava inserida
            }

            string novoRestante = restante.substr(filho->rotulo.size());
            NoPatricia *resultado = removerAux(filho, novoRestante);

            if (resultado == nullptr) {
                no->filhos.erase(c); // filho foi totalmente apagado
            } else if (resultado != filho) {
                // filho foi fundido com o próprio neto: atualiza a entrada no mapa
                no->filhos.erase(c);
                no->filhos[resultado->rotulo[0]] = resultado;
            }
        }

        if (no == raiz) {
            return no; // a raiz nunca é apagada nem fundida
        }

        if (no->filhos.empty() && !no->fimDePalavra) {
            delete no;
            return nullptr;
        }

        if (no->filhos.size() == 1 && !no->fimDePalavra) {
            NoPatricia *unico = no->filhos.begin()->second;
            unico->rotulo = no->rotulo + unico->rotulo;
            no->filhos.clear(); // evita apagar o filho junto no delete abaixo
            delete no;
            return unico;
        }

        return no;
    }

public:
    Patricia() {
        raiz = new NoPatricia("", false);
    }

    ~Patricia() {
        liberar(raiz);
    }

    void inserir(const string &palavra) {
        NoPatricia *atual = raiz;
        string restante = palavra;

        while (true) {
            if (restante.empty()) {
                atual->fimDePalavra = true;
                return;
            }

            char c = restante[0];
            auto it = atual->filhos.find(c);

            if (it == atual->filhos.end()) {
                atual->filhos[c] = new NoPatricia(restante, true);
                return;
            }

            NoPatricia *filho = it->second;
            string comum = prefixoComum(restante, filho->rotulo);

            if (comum == filho->rotulo) {
                restante = restante.substr(comum.size());
                atual = filho;
                continue;
            }

            NoPatricia *intermediario = new NoPatricia(comum, false);
            filho->rotulo = filho->rotulo.substr(comum.size());
            intermediario->filhos[filho->rotulo[0]] = filho;

            string restoNovaPalavra = restante.substr(comum.size());
            if (restoNovaPalavra.empty()) {
                intermediario->fimDePalavra = true;
            } else {
                intermediario->filhos[restoNovaPalavra[0]] = new NoPatricia(restoNovaPalavra, true);
            }

            atual->filhos[c] = intermediario;
            return;
        }
    }

    bool buscar(const string &palavra) {
        NoPatricia *atual = raiz;
        string restante = palavra;

        while (!restante.empty()) {
            char c = restante[0];
            auto it = atual->filhos.find(c);
            if (it == atual->filhos.end()) {
                return false;
            }

            NoPatricia *filho = it->second;
            if (restante.compare(0, filho->rotulo.size(), filho->rotulo) != 0) {
                return false;
            }

            restante = restante.substr(filho->rotulo.size());
            atual = filho;
        }

        return atual->fimDePalavra;
    }

    void remover(const string &palavra) {
        removerAux(raiz, palavra);
    }

private:
    // Percorre a arvore concatenando os rotulos das arestas ate cada
    // folha/no marcado como fim de palavra (usado pela exportacao .txt)
    void coletarPalavras(NoPatricia *no, string prefixoAcumulado, vector<string> &saida) {
        string caminho = prefixoAcumulado + no->rotulo;
        if (no->fimDePalavra) saida.push_back(caminho);
        for (auto &par : no->filhos) {
            coletarPalavras(par.second, caminho, saida);
        }
    }

    // gera as linhas do .dot. Diferente da Trie, aqui o rotulo da ARESTA e'
    // o "rotulo" (string, nao letra unica) armazenado no proprio NO filho --
    // e' exatamente essa compactacao de trechos sem ramificacao em uma unica
    // aresta que diferencia a Patricia da Trie. Nos que fecham palavra saem
    // com contorno duplo.
    int exportarDot(NoPatricia *no, ofstream &arquivo, int &contador) {
        int idAtual = contador++;
        if (no->fimDePalavra) {
            arquivo << "    n" << idAtual << " [label=\"\", shape=doublecircle, fillcolor=\"#ffd699\"];\n";
        } else {
            arquivo << "    n" << idAtual << " [label=\"\"];\n";
        }
        for (auto &par : no->filhos) {
            int idFilho = exportarDot(par.second, arquivo, contador);
            arquivo << "    n" << idAtual << " -> n" << idFilho
                    << " [label=\"" << par.second->rotulo << "\"];\n";
        }
        return idAtual;
    }

public:
    vector<string> listarPalavras() {
        vector<string> saida;
        coletarPalavras(raiz, "", saida);
        return saida;
    }

    // Exporta a arvore atual para um arquivo .dot (formato Graphviz),
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
        arquivo << "// Arvore Patricia -- gerado automaticamente\n";
        arquivo << "digraph PatriciaTree {\n";
        arquivo << "    label=\"" << titulo
                << "\\n(circulo duplo laranja = fim de palavra; rotulo da aresta = trecho compactado)\";\n";
        arquivo << "    labelloc=t;\n";
        arquivo << "    node [shape=circle, style=filled, fillcolor=\"#e0e0e0\", fixedsize=true, width=0.4];\n";
        int contador = 0;
        exportarDot(raiz, arquivo, contador);
        arquivo << "}\n";
        cout << "Arvore exportada para \"" << caminho << "\" (renderize com: dot -Tpng "
             << caminho << " -o " << caminho.substr(0, caminho.find_last_of('.')) << ".png)\n";
    }
};

int main(int argc, char* argv[]) {
    Patricia arvore;
    using Relogio = chrono::high_resolution_clock;
    const char* ARQUIVO_RESULTADOS = "resultados/resultados.csv";

    // "input/dados_trie_patricia.dat" e relativo ao diretorio de onde o
    // executavel e rodado -- execute a partir da pasta EmCpp. Aceita
    // opcionalmente o caminho do arquivo como argumento (mesma convencao
    // usada em Splay/Treap/KDTree/Trie).
    string caminhoDados = (argc > 1) ? argv[1] : "input/dados_trie_patricia.dat";
    vector<string> palavrasIniciais = lerPalavras(caminhoDados);

    auto inicioInsercao = Relogio::now();
    for (const string &p : palavrasIniciais) arvore.inserir(p);
    auto fimInsercao = Relogio::now();
    double tempoInsercaoMs = chrono::duration<double, milli>(fimInsercao - inicioInsercao).count();
    cout << palavrasIniciais.size() << " palavras carregadas do arquivo de entrada.\n";
    cout << "  tempo de insercao: " << tempoInsercaoMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "Patricia", "palavras", "insercao", (long long)palavrasIniciais.size(), tempoInsercaoMs);

    auto inicioBusca = Relogio::now();
    int encontrados = 0;
    for (const string &p : palavrasIniciais) if (arvore.buscar(p)) encontrados++;
    auto fimBusca = Relogio::now();
    double tempoBuscaMs = chrono::duration<double, milli>(fimBusca - inicioBusca).count();
    cout << "  tempo de busca (" << palavrasIniciais.size() << " buscas, " << encontrados << " encontradas): " << tempoBuscaMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "Patricia", "palavras", "busca", (long long)palavrasIniciais.size(), tempoBuscaMs);

    size_t metade = palavrasIniciais.size() / 2;
    auto inicioRemocao = Relogio::now();
    for (size_t i = 0; i < metade; i++) arvore.remover(palavrasIniciais[i]);
    auto fimRemocao = Relogio::now();
    double tempoRemocaoMs = chrono::duration<double, milli>(fimRemocao - inicioRemocao).count();
    cout << "  tempo de remocao (" << metade << " palavras): " << tempoRemocaoMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "Patricia", "palavras", "remocao", (long long)metade, tempoRemocaoMs);

    // reinsere o que foi removido no benchmark, pro menu interativo abaixo
    // comecar com a arvore completa de novo
    for (size_t i = 0; i < metade; i++) arvore.inserir(palavrasIniciais[i]);

    // ---------- Demonstracao automatica (sem menu interativo) ----------
    // Antes, essa parte era um menu (cin >> opcao) que dependia de
    // digitacao manual. Agora roda tudo sozinho, igual as outras 4
    // estruturas: insere um conjunto pequeno e fixo de palavras com
    // prefixos compartilhados -- ideal pra evidenciar a compactacao/divisao
    // de prefixos da Patricia --, demonstra busca e remocao, e exporta o
    // resultado final pra um .txt.
    cout << "\n\n===== Demonstracao automatica (exemplo pequeno p/ Secao 3) =====\n";
    Patricia exemplo;
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

    cout << "\nRemovendo \"carro\" (mas \"carroca\" compartilha o prefixo comprimido e deve continuar existindo)...\n";
    exemplo.exportarDot("resultados/diagramas/patricia_antes.dot", "Patricia - ANTES de remover \\\"carro\\\"");
    exemplo.remover("carro");
    exemplo.exportarDot("resultados/diagramas/patricia_depois.dot", "Patricia - DEPOIS de remover \\\"carro\\\"");
    cout << "Busca por \"carro\" apos remocao: "
         << (exemplo.buscar("carro") ? "encontrada" : "nao encontrada") << "\n";
    cout << "Busca por \"carroca\" apos a remocao de \"carro\": "
         << (exemplo.buscar("carroca") ? "encontrada" : "nao encontrada") << "\n";

    // exporta a lista final de palavras da arvore de exemplo (substitui a
    // antiga opcao 4 do menu, que exigia digitar o nome do arquivo na mao)
    const string arquivoExportado = "resultados/patricia_exemplo.txt";
    ofstream arquivoSaida(arquivoExportado);
    if (arquivoSaida.is_open()) {
        vector<string> palavrasFinais = exemplo.listarPalavras();
        arquivoSaida << "===== Palavras armazenadas na Patricia de exemplo (" << palavrasFinais.size() << ") =====\n\n";
        for (const string &p : palavrasFinais) arquivoSaida << p << "\n";
        cout << "\nLista de palavras exportada para \"" << arquivoExportado << "\".\n";
    } else {
        cerr << "Aviso: nao foi possivel criar \"" << arquivoExportado << "\".\n";
    }

    return 0;
}