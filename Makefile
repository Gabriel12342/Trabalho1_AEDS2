# Makefile -- Trabalho Pratico Individual I: Estruturas em Arvores Avancadas
#
# Compila as 5 estruturas (Splay, Treap, Trie, Patricia, KDTree). Cada uma
# tem seu proprio main() num .cpp separado, e todas compartilham o
# Arvores.h / Padronizar.cpp (leitura/normalizacao dos arquivos de dados
# e a funcao gravarTempo() usada nas medicoes de desempenho).
#
# Uso:
#   make            -> compila as 5 estruturas em build/
#   make splay      -> compila so a Splay (idem treap, trie, patricia, kdtree)
#   make run        -> compila (se preciso) e roda as 5, em sequencia, de
#                      forma totalmente automatica (nenhuma delas depende de
#                      entrada via teclado), gerando as medicoes em
#                      resultados/resultados.csv
#   make clean      -> remove build/ e resultados/
#
# Os executaveis esperam ser rodados de dentro desta pasta (Trabalho1/),
# pois leem os arquivos de dados a partir de "input/...". Rodar via
# "make run" ja garante isso.

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall
BUILD    := build
COMUM    := Padronizar.cpp Arvores.h

# nome do executavel : arquivo .cpp com o main() correspondente
ALVOS    := $(BUILD)/Splay $(BUILD)/Treap $(BUILD)/Trie $(BUILD)/Patricia $(BUILD)/KDTree

.PHONY: all splay treap trie patricia kdtree run clean

all: $(ALVOS)

splay:    $(BUILD)/Splay
treap:    $(BUILD)/Treap
trie:     $(BUILD)/Trie
patricia: $(BUILD)/Patricia
kdtree:   $(BUILD)/KDTree

# cria a pasta build/ antes de qualquer coisa que dependa dela
$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/Splay: Splay.cpp $(COMUM) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ Splay.cpp Padronizar.cpp

$(BUILD)/Treap: Treap.cpp $(COMUM) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ Treap.cpp Padronizar.cpp

# o arquivo fonte da Trie se chama AvoreTrie.cpp, mas o executavel sai como
# "Trie" (mais claro pro professor identificar)
$(BUILD)/Trie: AvoreTrie.cpp $(COMUM) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ AvoreTrie.cpp Padronizar.cpp

# idem: fonte "patricia.cpp" (minusculo), executavel "Patricia"
$(BUILD)/Patricia: patricia.cpp $(COMUM) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ patricia.cpp Padronizar.cpp

$(BUILD)/KDTree: KDTree.cpp $(COMUM) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ KDTree.cpp Padronizar.cpp

# roda as 5 em sequencia (Trie e Patricia sao interativas -- "echo 0" fecha
# o menu delas automaticamente logo apos o benchmark inicial)
run: all
	@echo "===== Splay =====";    ./$(BUILD)/Splay
	@echo "===== Treap =====";    ./$(BUILD)/Treap
	@echo "===== Trie =====";     ./$(BUILD)/Trie
	@echo "===== Patricia ====="; ./$(BUILD)/Patricia
	@echo "===== KDTree =====";   ./$(BUILD)/KDTree

clean:
	rm -rf $(BUILD) resultados