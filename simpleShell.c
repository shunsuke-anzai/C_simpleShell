/*----------------------------------------------------------------------------
 *  簡易版シェル
 *--------------------------------------------------------------------------*/

/*
 *  インクルードファイル
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

/*
 *  定数の定義
 */

#define BUFLEN 1024   /* コマンド用のバッファの大きさ */
#define MAXARGNUM 256 /* 最大の引数の数 */
#define STACK_SIZE 10
#define PAST_SIZE 32
#define ALIAS_SIZE 10
/*
 *  ローカルプロトタイプ宣言
 */

int parse(char[], char *[]);
void execute_command(char *[], int);
void change_directory(char *args);
void push_directory();
void print_directory_stack();
void pop_directory();
void history();
void exclamation();
void string(char *args);
void prompt(char *args);
void alias(char *args[]);
void unalias(char *args);
void wild(char *args[]);
int blackjack();

typedef struct {
    char command1[BUFLEN];
    char command2[BUFLEN];
} Alias;

char *directory_stack[STACK_SIZE]; /* ディレクトリスタック */
int stack_index = 0;
char past_command[PAST_SIZE][BUFLEN];
int past_index = 0;
char prompt_string[BUFLEN] = "Command"; 
Alias alias_command[ALIAS_SIZE];
int alias_count = 0;
/*----------------------------------------------------------------------------
 *
 *  関数名   : main
 *
 *  作業内容 : シェルのプロンプトを実現する
 *
 *  引数     :
 *
 *  返り値   :
 *
 *  注意     :
 *
 *--------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  char command_buffer[BUFLEN]; /* コマンド用のバッファ */
  char *args[MAXARGNUM];       /* 引数へのポインタの配列 */
  int command_status;          
  int script_flag=0;
  int eof_flag=0;
  /* コマンドの状態を表す
  
                                  command_status = 0 : フォアグラウンドで実行
                                  command_status = 1 : バックグラウンドで実行
                                  command_status = 2 : シェルの終了
                                  command_status = 3 : 何もしない */

  /*
   *  無限にループする
   */
  if(isatty(fileno(stdin))==0){
    script_flag = 1;
  }
  for (;;) {
    
    /*
     *  プロンプトを表示する
     */

    /*
     *  標準入力から１行を command_buffer へ読み込む
     *  入力が何もなければ改行を出力してプロンプト表示へ戻る
     */
    if(eof_flag == 0){
        printf("%s : ",prompt_string);
    }

    if(fgets(command_buffer,BUFLEN,stdin)==NULL){
      command_status = 2;
    }else{
      if(script_flag == 1){
        if(feof(stdin)!=0){
          strcat(command_buffer,"\n");
          eof_flag=1;
        }
        printf("%s",command_buffer);
      }
      //ヒストリーに保存
      if(past_index < PAST_SIZE){
        strcpy(past_command[past_index], command_buffer);
      }else{
        for(int i=0; i < PAST_SIZE-1; i++){
          strcpy(past_command[i], past_command[i+1]);
        }
        strcpy(past_command[PAST_SIZE-1], command_buffer);
      }
      past_index++;
      if(command_buffer[0] =='!' && command_buffer[1] != '!'){
      int k = strlen(command_buffer);
      command_buffer[k+1]='\0';
      for(int x=k-1;x>0;x--){
        command_buffer[x+1]=command_buffer[x];
      }
      command_buffer[1]=' ';
      }
      command_status = parse(command_buffer, args);
    }
    
    if(alias_count>0){
      for(int j=0;j<alias_count;j++){
        if(strcmp(alias_command[j].command1, args[0])==0){
        args[0]=alias_command[j].command2;
        }
      }
    }

    /*
     *  終了コマンドならばプログラムを終了
     *  引数が何もなければプロンプト表示へ戻る
     */
    if (command_status == 2) {
      printf("done.\n");
      exit(EXIT_SUCCESS);
    } else if (command_status == 3) {
      continue;
    } 
        execute_command(args, command_status);
  }
  return 0;
}

/*----------------------------------------------------------------------------
 *
 *  関数名   : parse
 *
 *  作業内容 : バッファ内のコマンドと引数を解析する
 *
 *  引数     :
 *
 *  返り値   : コマンドの状態を表す :
 *                0 : フォアグラウンドで実行
 *                1 : バックグラウンドで実行
 *                2 : シェルの終了
 *                3 : 何もしない
 *
 *  注意     :
 *
 *--------------------------------------------------------------------------*/

int parse(char buffer[], /* バッファ */
          char *args[])  /* 引数へのポインタ配列 */
{
  int arg_index; /* 引数用のインデックス */
  int status;    /* コマンドの状態を表す */

  /*
   *  変数の初期化
   */

  arg_index = 0;
  status = 0;

  /*
   *  バッファ内の最後にある改行をヌル文字へ変更
   */

  *(buffer + (strlen(buffer) - 1)) = '\0';

  /*
   *  バッファが終了を表すコマンド（"exit"）ならば
   *  コマンドの状態を表す返り値を 2 に設定してリターンする
   */

  if (strcmp(buffer, "exit") == 0) {

    status = 2;
    return status;
  }

  /*
   *  バッファ内の文字がなくなるまで繰り返す
   *  （ヌル文字が出てくるまで繰り返す）
   */

  while (*buffer != '\0') {

    /*
     *  空白類（空白とタブ）をヌル文字に置き換える
     *  これによってバッファ内の各引数が分割される
     */

    while (*buffer == ' ' || *buffer == '\t') {
      *(buffer++) = '\0';
    }

    /*
     * 空白の後が終端文字であればループを抜ける
     */

    if (*buffer == '\0') {
      break;
    }

    /*
     *  空白部分は読み飛ばされたはず
     *  buffer は現在は arg_index + 1 個めの引数の先頭を指している
     *
     *  引数の先頭へのポインタを引数へのポインタ配列に格納する
     */

    args[arg_index] = buffer;
    ++arg_index;

    /*
     *  引数部分を読み飛ばす
     *  （ヌル文字でも空白類でもない場合に読み進める）
     */

    while ((*buffer != '\0') && (*buffer != ' ') && (*buffer != '\t')) {
      ++buffer;
    }
  }

  /*
   *  最後の引数の次にはヌルへのポインタを格納する
   */

  args[arg_index] = NULL;

  /*
   *  最後の引数をチェックして "&" ならば
   *
   *  "&" を引数から削る
   *  コマンドの状態を表す status に 1 を設定する
   *
   *  そうでなければ status に 0 を設定する
   */

  if (arg_index > 0 && strcmp(args[arg_index - 1], "&") == 0) {

    --arg_index;
    args[arg_index] = '\0';
    status = 1;

  } else {

    status = 0;
  }

  /*
   *  引数が何もなかった場合
   */

  if (arg_index == 0) {
    status = 3;
  }

  /*
   *  コマンドの状態を返す
   */

  return status;
}

/*----------------------------------------------------------------------------
 *
 *  関数名   : execute_command
 *
 *  作業内容 : 引数として与えられたコマンドを実行する
 *             コマンドの状態がフォアグラウンドならば、コマンドを
 *             実行している子プロセスの終了を待つ
 *             バックグラウンドならば子プロセスの終了を待たずに
 *             main 関数に返る（プロンプト表示に戻る）
 *
 *  引数     :
 *
 *  返り値   :
 *
 *  注意     :
 *
 *--------------------------------------------------------------------------*/

void execute_command(char *args[], int command_status)
{
  char *extracted_string = args[0] + 1;
  for(int i=1;args[i]!=NULL;i++){
    if(strcmp(args[i], "*") == 0){
      wild(args);
      break;
    }
  }

  if (strcmp(args[0], "cd") == 0){
        change_directory(args[1]);
    }else if (strcmp(args[0], "pushd") == 0) {
        push_directory();
    } else if (strcmp(args[0], "dirs") == 0) {
        print_directory_stack(); 
    } else if (strcmp(args[0], "popd") == 0) {
        pop_directory(); 
    } else if (strcmp(args[0], "history") == 0){
        history();
    } else if (strcmp(args[0], "!!") == 0){
        exclamation();
    } else if(strcmp(args[0], "!") == 0){
        string(args[1]);
    } else if(strcmp(args[0], "prompt") == 0){
        prompt(args[1]);
    } else if(strcmp(args[0], "alias") == 0){
        alias(args);
    } else if(strcmp(args[0], "unalias") == 0){
        unalias(args[1]);
    } else if(strcmp(args[0], "bj")==0){
        blackjack();
    } else{
      pid_t pid;   /* プロセスID */
      int status;  /* 子プロセスの終了ステータス */

      pid = fork();  /* 新しいプロセスを生成 */

      if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
      } else if (pid == 0) {
        /* 子プロセスの場合 */

        /* 引数として与えられたコマンドを実行 */
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        /* execvpが成功すればここには到達しない */
      } else {
        /* 親プロセスの場合 */

        if (command_status == 0) {
            /* コマンドの状態がフォアグラウンドの場合、子プロセスの終了を待つ */
            waitpid(pid, &status, 0);
        } else {
            /* コマンドの状態がバックグラウンドの場合、待たずにプロンプトに戻る */
            printf("Background process started with PID %d\n", pid);
        }
    }
  }
  return;
}

void change_directory(char *args) {
    int result;
    if (args == NULL) {
        /* 引数がない場合は親ディレクトリに移動する */
        result = chdir("..");
    } else {
        /* 引数がある場合は指定されたディレクトリに移動する */
        result = chdir(args);
    }
    if (result == -1) {
        perror("chdir");
    }
    return;
}

void push_directory(const char *dir) {
    if (stack_index >= STACK_SIZE) {
        printf("エラー: ディレクトリスタックが一杯です。これ以上ディレクトリを追加できません。\n");
        return;
    }
    char *current_dir = getcwd(NULL, 0);
    if (current_dir == NULL) {
        perror("getcwd");
        return;
    }
    directory_stack[stack_index] = current_dir;
    stack_index++;
    return;
}

void print_directory_stack() {
    if (stack_index == 0) {
        printf("ディレクトリスタックは空です。\n");
        return;
    }

    for (int i = stack_index - 1; i >=0; i--) {
        printf("%s\n", directory_stack[i]);
    }
  return;
}

void pop_directory() {
    if (stack_index == 0) {
        printf("エラー: ディレクトリスタックは空です。ポップできません。\n");
        return;
    }
    int result;
    result = chdir(directory_stack[stack_index-1]);
    if (result == -1) {
        perror("chdir");
    } else {
        free(directory_stack[stack_index - 1]);
        stack_index--;
    }
    return;
}

void history(){
    for (int i = 0; i < past_index; i++) {
        printf("%d: %s", i + 1, past_command[i]);
    }
  return;
}

void exclamation(){
  if(past_index==0){
    printf("過去に実行したコマンドはありません。\n");
  }else{
    char *last_command;
    if(past_index < PAST_SIZE){
      strcpy(past_command[past_index-1], past_command[past_index-2]);
      last_command = past_command[past_index - 2];
    }else{
        for(int i=0; i < PAST_SIZE-1; i++){
          strcpy(past_command[i], past_command[i+1]);
        }
        strcpy(past_command[PAST_SIZE-1], past_command[PAST_SIZE-2]);
        last_command = past_command[PAST_SIZE - 1];
    }
    char *args2[MAXARGNUM];
    int command_status = parse(last_command , args2);
    execute_command(args2, command_status);
  }
  return;
}

void string(char *args){
  if(past_index==0){
    printf("過去に実行したコマンドはありません。\n");
  }else{
    int k =strlen(args);
    char string_command[BUFLEN];
    char *last_command;
    for(int l=past_index;l>=0;l--){
      if (strncmp(past_command[l], args, k)==0){
        if(past_index < PAST_SIZE){
          strcpy(past_command[past_index-1], past_command[l]);
          last_command = past_command[l];
        }else{
          for(int i=0; i < PAST_SIZE-1; i++){
            strcpy(past_command[i], past_command[i+1]);
          }
          strcpy(past_command[PAST_SIZE-1], past_command[PAST_SIZE-2]);
          last_command = past_command[PAST_SIZE - 1];
        }
        char *args2[MAXARGNUM];
        int command_status = parse(last_command , args2);
        execute_command(args2, command_status);
        return;
      }
    }
    printf("その文字列で始まるコマンドはヒストリに格納されていません。");
  }
  return;
}

void prompt(char *args){
  if (args != NULL) {
    strcpy(prompt_string, args); // 引数があればプロンプト文字列を更新
  } else {
    strcpy(prompt_string, "Command"); // 引数がなければデフォルトに戻す
  }
}

void alias(char *args[]){
  if(args[1] == NULL || args[2] == NULL){
    if(alias_count==0){
      printf("登録されたコマンドはありません。\n");
    }else{
      for(int i=0; i < alias_count;i++){ 
        printf("%s %s\n",alias_command[i].command1,alias_command[i].command2);
      }
    }
  } else if (args[2] == NULL){
    printf("エラー:置き換えたいコマンド名を入力してください。");
  } else {
    strcpy(alias_command[alias_count].command1, args[1]);
    strcpy(alias_command[alias_count].command2, args[2]);
    alias_count++;
  }
}

void unalias(char *args){
  for(int i=0;i<alias_count;i++){
    if(strcmp(alias_command[i].command1, args)==0){
      for(int j = i; j < alias_count - 1; j++){
        strcpy(alias_command[j].command1, alias_command[j + 1].command1);
        strcpy(alias_command[j].command2, alias_command[j + 1].command2);
      }
        alias_count--;
        return;      
    }
  }
  printf("その名前で登録したコマンドはありません。");
}

void wild(char *args[]) {
  DIR *dir;
  struct dirent *entry;
  char file[BUFLEN] = "\0";
  char new_args[BUFLEN] = "\0";
  if ((dir = opendir(".")) == NULL) {
    perror("opendir");
    return;
  }
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) {
      strcat(file, entry->d_name);
      strcat(file, " ");
    }
  }
  strcat(file, "\0");
  for (int k = 0; args[k] != NULL; k++) {
    if (strstr(args[k], "*") !=
        NULL) {
      strcat(new_args, file);
    } else {
      strcat(new_args, args[k]);
      strcat(new_args, " ");
    }
  }
  strcat(new_args, "\0");
  int i = parse(new_args, args);
  if (closedir(dir)) {
    perror("closedir");
    return;
  }
  printf("\n");
  return;
}

// 数値をJ, Q, Kに変換する関数
const char* convert_value_to_string(int value) {
    static const char* card_values[] = { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
    return card_values[value - 1];
}

// 数値を10以下の値に変換する関数（11以上は10として扱う）
int convert_to_10_or_less(int value) {
    return (value > 10) ? 10 : value;
}

int blackjack() {
    srand(time(NULL));

    int Dealer[5]; // Dealerの手札（最大5枚）を格納する配列
    int Player[5]; // Playerの手札（最大5枚）を格納する配列
    int dealerTotal = 0; // Dealerの手札の合計値
    int playerTotal = 0; // Playerの手札の合計値
    int numOfCardsDealtDealer = 2; // Dealerの手札枚数
    int numOfCardsDealtPlayer = 2; // Playerの手札枚数

    // 初期手札の配布
    for (int i = 0; i < numOfCardsDealtDealer; i++) {
        Dealer[i] = rand() % 13 + 1;
        dealerTotal += convert_to_10_or_less(Dealer[i]);
    }

    for (int i = 0; i < numOfCardsDealtPlayer; i++) {
        Player[i] = rand() % 13 + 1;
        playerTotal += convert_to_10_or_less(Player[i]);
    }
    printf("---[BLACK JACK]---   GAME START!!\n");
    // 初期手札の表示
    printf("Dealer's hands: %s, ?\n", convert_value_to_string(Dealer[0]));
    printf("Player's hands: %s, %s\n", convert_value_to_string(Player[0]), convert_value_to_string(Player[1]));

    // プレイヤーのターン
    while (1) {
        char choice;
        printf("\nINPUT 'h' or 's'（h=Hit, s=Stand）: ");
        int num_items_read = scanf(" %c", &choice);
        getchar(); // 改行文字をクリア
        if (num_items_read != 1) {
        printf("入力エラー\n");
        break;
        }

        if (choice == 'h' || choice == 'H') {
            // Hitの場合、Playerに1つ乱数を追加
            Player[numOfCardsDealtPlayer] = rand() % 13 + 1;
            printf("\nPlayer's hands: ");
            for (int i = 0; i <= numOfCardsDealtPlayer; i++) {
                printf("%s ", convert_value_to_string(Player[i]));
            }
            printf("\n");

            playerTotal += convert_to_10_or_less(Player[numOfCardsDealtPlayer]);
            numOfCardsDealtPlayer++;

            // バーストの判定
            if (playerTotal > 21) {
                printf("Burst!\n");
                break;
            }
        } else if (choice == 's' || choice == 'S') {
            // Standの場合、ディーラーのターンへ
            // Aを11として扱った場合の処理
            int numOfAces = 0;
            playerTotal = 0;
            for (int i = 0; i < numOfCardsDealtPlayer; i++) {
              if (Player[i] == 1) {
                numOfAces++;
              }
            playerTotal += convert_to_10_or_less(Player[i]);
            }

            if (numOfAces > 0 && playerTotal + 10 <= 21) {
              playerTotal += 10;
            }
            printf("\nPlayer Total: %d\n", playerTotal);
            break;
        }
    }

    // ディーラーのターン
    while (dealerTotal < 17) {
        // Dealerに1つ乱数を追加
        Dealer[numOfCardsDealtDealer] = rand() % 13 + 1;
        dealerTotal += convert_to_10_or_less(Dealer[numOfCardsDealtDealer]);
        numOfCardsDealtDealer++;

        // Aを11として扱った場合の処理
        if (Dealer[numOfCardsDealtDealer - 1] == 1 && dealerTotal <= 11) {
            dealerTotal += 10;
        }
    }

    // 勝敗の判定
    printf("\n[Result]\n");
    printf("Dealer's hands: ");
    for (int i = 0; i < numOfCardsDealtDealer; i++) {
        printf("%s ", convert_value_to_string(Dealer[i]));
    }
    printf("\n");
    printf("Player's hands: ");
    for (int i = 0; i < numOfCardsDealtPlayer; i++) {
        printf("%s ", convert_value_to_string(Player[i]));
    }
    printf("\n");

    if (playerTotal > 21) {
        // プレイヤーがバーストしている場合
        printf("---Burst! Player Lose...---\n");
    } else if (dealerTotal > 21) {
        // ディーラーがバーストしている場合
        printf("---Dealer Burst! Player Win!---\n");
    } else if (playerTotal == dealerTotal) {
        // 引き分けの場合
        printf("---Draw---\n");
    } else if (playerTotal > dealerTotal) {
        // プレイヤーの手札がディーラーの手札よりも大きい場合
        printf("---Player Win!---\n");
    } else {
        // ディーラーの手札がプレイヤーの手札よりも大きい場合
        printf("---Player Lose...---\n");
    }
}
/*-- END OF FILE -----------------------------------------------------------*/
