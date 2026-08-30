#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <emscripten.h>
using namespace std;
constexpr int BOARD_SIZE=8;
constexpr int BOARD_ARR_LEN=BOARD_SIZE+1;
constexpr int MAX_DEP=10;
constexpr bool human_color=1;
constexpr bool AI_color=human_color^1;
constexpr int INF=1e9;
constexpr int dx[BOARD_SIZE]={-1,1,-1,1,1,0,-1,0};
constexpr int dy[BOARD_SIZE]={-1,1,1,-1,0,1,0,-1};
struct Board{
    vector<vector<int> > board;//-1 empty, 0 white, 1 black
    vector<int> sum_piece;
    bool turn;
    pair<int,int> lst_pos;
    Board(){
        board=vector<vector<int> >(BOARD_ARR_LEN,vector<int>(BOARD_ARR_LEN));
        sum_piece.resize(2);
        turn=0;
        lst_pos={0,0};
    }
    void pass(){
        turn^=1;
    }
    void start(){
        turn=1;sum_piece[0]=sum_piece[1]=2;
        for(int i=1;i<=BOARD_SIZE;i++){
            for(int j=1;j<=BOARD_SIZE;j++) board[i][j]=-1;
        }
        board[4][4]=board[5][5]=1;
        board[4][5]=board[5][4]=0;
    }
    bool is_end()const{
        return !sum_piece[0]||!sum_piece[1]||(sum_piece[0]+sum_piece[1]==BOARD_SIZE*BOARD_SIZE);
    }
    bool in_board(int pos){
        return 1<=pos&&pos<=BOARD_SIZE;
    }
    int calculate(int turn){//计算turn相比turn^1的优势
        return sum_piece[turn]-sum_piece[turn^1];
    }
    bool place_piece(pair<int,int> pos){
        int pos_x=pos.first,pos_y=pos.second;
        if(!in_board(pos_x)||!in_board(pos_y)) return 0;
        if(board[pos_x][pos_y]!=-1) return 0;
        sum_piece[turn]++;
        board[pos_x][pos_y]=turn;
        
        lst_pos={pos_x,pos_y};
        bool is_flip=0;

        for(int i=0;i<8;i++){
            int new_x=pos_x,new_y=pos_y;
            new_x+=dx[i],new_y+=dy[i];
            while(1){
                if(!in_board(new_x)||!in_board(new_y)) break;
                int color=board[new_x][new_y];
                if(color==-1) break;
                else if(color==turn){
                    new_x-=dx[i];new_y-=dy[i];
                    while(new_x!=pos_x||new_y!=pos_y){
                        is_flip=1;
                        board[new_x][new_y]=turn;
                        sum_piece[turn]++;sum_piece[turn^1]--;
                        new_x-=dx[i],new_y-=dy[i];
                    }
                    break;
                }
                new_x+=dx[i],new_y+=dy[i];
            }
        }
        turn^=1;
        return is_flip;
    }
};
struct CmdInOut{
    void print_board(const Board &board){
        cerr<<"  ";
        for(int i=1;i<=BOARD_SIZE;i++) cerr<<i<<" ";
        cerr<<'\n';
        for(int i=1;i<=BOARD_SIZE;i++){
            cerr<<i<<" ";
            for(int j=1;j<=BOARD_SIZE;j++){
                if(board.board[i][j]==-1) cerr<<"- ";
                else cerr<<board.board[i][j]<<' ';
            }
            cerr<<'\n';
        }
    }
    void output_sum(const Board &board){
        cerr<<"Black: "<<board.sum_piece[1]<<"  "<<"White: "<<board.sum_piece[0]<<'\n';
    }
}cmd;
struct AIOperation{
    pair<int,int> AI_pos;
    int minimax_search(Board board,int alpha,int beta,int dep,bool is_max,bool is_rt){//turn该谁落子，永远求 max
        if(dep==MAX_DEP||board.is_end()) return board.calculate(AI_color);

        bool is_place=0;
        for(int i=1;i<=BOARD_SIZE&&alpha<beta;i++){
            for(int j=1;j<=BOARD_SIZE&&alpha<beta;j++){
                Board new_board=board;
                if(!new_board.place_piece({i,j})) continue;
                is_place=1;
                int pt=minimax_search(new_board,alpha,beta,dep+1,is_max^1,false);
                if(is_max){
                    if(is_rt&&pt>alpha) AI_pos={i,j};
                    alpha=max(alpha,pt);
                }
                else{
                    beta=min(beta,pt);
                }
            }
        }
        if(!is_place){
            board.pass();
            return minimax_search(board,alpha,beta,dep+1,is_max^1,false);
        }
        if(is_max) return alpha;
        else return beta;
    }
    pair<int,int> AI_place(Board board){
        AI_pos={0,0};
        minimax_search(board,-INF,INF,1,true,true);
        if(board.place_piece(AI_pos)) return AI_pos;
        else return {0,0};
    }
}AI;
Board board;
void init(){
    board.start();
}

// ========== JS导出接口 extern "C" EMSCRIPTEN_KEEPALIVE ==========
extern "C" {

EMSCRIPTEN_KEEPALIVE
void c_init() {
    init();
}

// 输入一份board副本，返回是否落子成功
EMSCRIPTEN_KEEPALIVE
int c_put_piece(int pos,int* ptr,int turn,int sum0,int sum1) {
    int k=0;
    for(int i=1;i<=BOARD_SIZE;i++){
        for(int j=1;j<=BOARD_SIZE;j++) board.board[i][j]=ptr[k++];
    }
    board.turn=turn;board.sum_piece[0]=sum0;board.sum_piece[1]=sum1;
    int pos_x=(pos-1)/BOARD_SIZE,pos_y=(pos-1)%BOARD_SIZE+1;
    if(pos_x==0&&pos_y==0){
        board.pass();
        return true;
    }
    Board new_board=board;
    if(!new_board.place_piece({pos_x,pos_y})) return false;
    board=new_board;
    return true;
}

EMSCRIPTEN_KEEPALIVE
int get_turn() {
    return (int)board.turn;
}

// 将board.board[1‑8][1‑8]拷贝到wasm堆ptr，JS读取，8*8=64个int
EMSCRIPTEN_KEEPALIVE
void get_board(int* ptr) {
    int k=0;
    for(int i=1;i<=8;i++){
        for(int j=1;j<=8;j++){
            ptr[k++] = board.board[i][j];
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int get_is_end() {
    return (int)board.is_end();
}

// sum_piece[0],sum_piece[1]写入ptr[0],ptr[1]
EMSCRIPTEN_KEEPALIVE
void get_sum_piece(int* ptr) {
    ptr[0] = board.sum_piece[0];
    ptr[1] = board.sum_piece[1];
}

// AI计算函数，给worker使用，输入一份board副本，返回pos(1‑64，0代表pass)
EMSCRIPTEN_KEEPALIVE
int worker_ai_calc(int* ptr,int turn,int sum0,int sum1){
    Board board;
    int k=0;
    for(int i=1;i<=BOARD_SIZE;i++){
        for(int j=1;j<=BOARD_SIZE;j++) board.board[i][j]=ptr[k++];
    }
    board.turn=turn;board.sum_piece[0]=sum0;board.sum_piece[1]=sum1;
    pair<int,int> pos = AI.AI_place(board);
    int x=pos.first,y=pos.second;
    if(x==0&&y==0){
        board.pass();
    }
    else{
        board.place_piece(pos);
    }
    return x*8+y;
}

} // extern C

int main(){
    return 0;
}