#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define OK 1
#define ERROR 0

typedef int Status;

#define MAX_WORD_STRING_LENGTH 100 // 字串的最大字数
#define MAX_WORD_LENGTH 10 // 单个词语的最大字数
#define MAX_LINE_LENGTH 50 // 词典一行的最大长度

// 有向网邻接矩阵的数据结构
typedef struct {
    int freq; // 词语在词典中出现的频数
    double fee; // 词语的"费用"
    char content[3 * MAX_WORD_LENGTH + 1]; // 词语的内容
} WordMatrix[MAX_WORD_STRING_LENGTH + 1][MAX_WORD_STRING_LENGTH + 1];

// 有向网的数据结构
typedef struct {
    WordMatrix words; // 邻接矩阵
    bool seg[MAX_WORD_STRING_LENGTH + 1]; // 分词标记数组
    int character_num; // 字数
    int word_num; // 词语数
} WordString;

FILE *dict = NULL; // 用来读取词典的文件指针
int total_word_freq = 0; // 词典中所有词语的总频数
int total_word_num = 0; // 词典中词语的总数

// 计算词典中词语的总数及所有词语的总频数
void CountTotalWordFreqAndNum() {
    while (!feof(dict)) {
        // 每次读取词典的一行，储存在line中
        char line[MAX_LINE_LENGTH];
        fgets(line, MAX_LINE_LENGTH, dict);

        // 将这一行的词语的频数加到词典总频数中
        int freq;
        sscanf(line, "%*s%d", &freq);
        total_word_freq += freq;

        // 词语总数加1
        ++total_word_num;
    }

    // 将文件指针恢复到指向词典开头
    rewind(dict);
}

// 查询词语在词典中的频数
int LookUpWordFreq(char *w) {
    while (!feof(dict)) {
        // 每次读取词典的一行，储存在line中
        char line[MAX_LINE_LENGTH];
        fgets(line, MAX_LINE_LENGTH, dict);

        // 读取这一行的词语，储存在word中
        char word[3 * MAX_WORD_LENGTH + 1];
        sscanf(line, "%s", word);

        // 若要查的词语与该行词语匹配上，则读取并返回该行词语的频数
        if (!strcmp(w, word)) {
            int freq;
            sscanf(line, "%*s%d", &freq);
            rewind(dict);
            return freq;
        }
    }

    // 将文件指针恢复到指向词典开头
    rewind(dict);

    // 若词典中查不到该词，则返回0
    return 0;
}

Status InitWordString(WordString &S) {
    // 初始化邻接矩阵
    memset(S.words, 0, sizeof(S.words));

    // 初始化分词标记数组
    for (int i = 0; i < MAX_WORD_STRING_LENGTH + 1; ++i) {
        S.seg[i] = false;
    }

    // 初始化字数和词数
    S.character_num = 0;
    S.word_num = 0;

    return OK;
}

Status CreateWordString(WordString &S, char s[]) {
    // 打开词典，失败则报错并退出程序
    dict = fopen("dict.txt", "r");
    if (dict == NULL) {
        printf("词典读取失败！\n");
        exit(ERROR);
    }

    // 计算词典中词语的总数及所有词语的总频数
    CountTotalWordFreqAndNum();

    // 计算字串的字数
    S.character_num = strlen(s) / 3;

    // 对每一个字，在词典中查询其频数。若查不到，则视作查到了，并赋予其频数0
    for (int i = 0; i < S.character_num; ++i) {
        strncpy(S.words[i][i + 1].content, s + 3 * i, 3);
        S.words[i][i + 1].freq = LookUpWordFreq(S.words[i][i + 1].content);

        // 若查不到，则视作查到了，因此将词典中词语总数加1
        if (!S.words[i][i + 1].freq) {
            ++total_word_num;
        }
    }

    // 对每一个词，在词典中查询其频数并依此计算其"费用"
    // 计算费用的方法为该词在词典中出现的概率的负自然对数，其中计算概率时采用拉普拉斯Add-one平滑
    for (int i = 0; i < S.character_num; ++i) {
        for (int j = 1; j < S.character_num + 1; ++j) {
            // 若词语并非单个字，那么词典中查得到则根据频数计算费用，查不到则将费用设为无穷
            if (j - i > 1) {
                strncpy(S.words[i][j].content, s + 3 * i, 3 * (j - i));
                S.words[i][j].freq = LookUpWordFreq(S.words[i][j].content);
                if (S.words[i][j].freq) {
                    ++S.word_num;
                    S.words[i][j].fee = log(total_word_freq + total_word_num) - log1p(S.words[i][j].freq);
                } else {
                    S.words[i][j].fee = INFINITY;
                }
            } else if (j - i == 1) {
                // 若词语为单个字，那么不必再重复查询频数，直接计算费用（词典中查不到的词也计算费用）
                ++S.word_num;
                S.words[i][j].fee = log(total_word_freq + total_word_num) - log1p(S.words[i][j].freq);
            }
        }
    }

    // 关闭词典
    fclose(dict);

    return OK;
}

Status SegmentWordString(WordString &S) {
    // 记录每个节点在最短路径上的前驱节点
    int prev[S.character_num + 1];
    for (int i = 1; i < S.character_num + 1; ++i) {
        prev[i] = 0;
    }

    // 记录到每个节点的当前距离
    double path_fee[S.character_num + 1];
    path_fee[0] = 0;
    for (int i = 1; i < S.character_num + 1; ++i) {
        path_fee[i] =  S.words[0][i].fee;
    }

    // 记录到每个节点的最短路径是否已被求出
    bool final[S.character_num + 1];
    final[0] = true;
    memset(final + 1, false, S.character_num);

    while (!final[S.character_num]) {
        int v;
        double min = INFINITY;

        // 遍历所有最小路径尚未求出的节点，找到费用最低的
        for (int i = 0; i < S.character_num + 1; ++i) {
            if (!final[i] && path_fee[i] < min) {
                v = i;
                min = path_fee[i];
            }
        }

        // 将其最短路径的状态改为"已找到"
        final[v] = true;

        // 更新到剩余节点的当前距离
        for (int i = 0; i < S.character_num + 1; ++i) {
            if (!final[i] && min + S.words[v][i].fee < path_fee[i]) {
                path_fee[i] = min + S.words[v][i].fee;
                prev[i] = v;
            }
        }
    }

    // 将位于从第一个节点到最后一个节点的最短路径上的节点的分词标记改为"是"
    int index = S.character_num;
    while (prev[index] != 0) {
        S.seg[prev[index]] = true;
        index = prev[index];
    }

    return OK;
}

Status PrintWordString(WordString S) {
    // 用来记录有分词标志的节点下标的数组
    int seg_indexes[S.character_num - 1];
    // 有分词标志的节点个数
    int seg_num = 0;

    // 遍历所有节点，找到有分词标志的节点，将其下标存入seg_indexes，每找到一个就使seg_num加1
    for (int i = 1; i < S.character_num; ++i) {
        if (S.seg[i]) {
            seg_indexes[seg_num++] = i;
        }
    }

    // 用来储存要输出的分词结果的字符串
    char w[4 * MAX_WORD_STRING_LENGTH] = {0};

    // 若有分词标志的节点数不为0，则依此将字串片段与"/"号连接到w上；若为0，则直接将整个字串复制到w中
    if (seg_num) {
        strcat(w, S.words[0][seg_indexes[0]].content);
        strcat(w, "/");
        for (int i = 0; i < seg_num - 1; ++i) {
            strcat(w, S.words[seg_indexes[i]][seg_indexes[i + 1]].content);
            strcat(w, "/");
        }
        strcat(w, S.words[seg_indexes[seg_num - 1]][S.character_num].content);
    } else {
        strcpy(w, S.words[0][S.character_num].content);
    }

    printf("%s\n", w);
}

Status DestroyWordString(WordString &S) {
    // 清空字串S使用的储存空间
    memset(&S, 0, sizeof(S));

    return OK;
}

int main() {
    WordString S;

    while (true) {
        InitWordString(S);

        printf("请输入待切分的字串：\n");
        printf(">>> ");

        // 用来储存用户输入的字串的字符串
        char s[3 * MAX_WORD_STRING_LENGTH + 1] = {0};
        scanf("%s", s);

        CreateWordString(S, s);
        SegmentWordString(S);

        printf("\n分词结果如下：\n");
        PrintWordString(S);

        DestroyWordString(S);

        printf("\n是否继续？\n");
        printf("继续......0 退出......1\n");
        printf(">>> ");

        int sel;
        scanf("%d", &sel);

        // 若用户选择退出，则跳出循环
        if (sel) {
            break;
        }

        putchar('\n');
    }

    return 0;
}