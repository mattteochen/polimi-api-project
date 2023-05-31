// Online C++ compiler to run C++ program online
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <limits.h>

using namespace std;

typedef struct {
    int idx;
    string asf;
    int jmps;
} Pair;

static inline int get_diff(int a, int b) {
    return (b-a) * 10;
}

static inline int get_original_idx(int idx) {
    return (1+idx)*10;
}

void get_path(int* dp, int* arr, int size) {
    queue<Pair> queue;
    queue.push(Pair{0, "", dp[0]});
    
    while (!queue.empty()) {
        Pair tmp = queue.front();
        queue.pop();
        
        if (!tmp.jmps) {
            cout << "0" << tmp.asf << endl;
            continue;
        }
        
        for (int i=0; i<=arr[tmp.idx]; i++) {
            if (tmp.idx+i < size && tmp.jmps-1 == dp[tmp.idx+i]) {
                queue.push(Pair{tmp.idx+i, (tmp.asf) + "->" + to_string(tmp.idx+i), tmp.jmps-1});
            }
        }
    }
}

void minJumps(int* dp, int* arr, int size){
    for(int idx=size-2; idx>=0; idx--){
        cout << endl << "index: " << idx << endl;
        int steps = arr[idx];
        cout << "curr steps: " << steps << endl;
    
        int min = INT_MAX;
        int min_idx = -1;
        if(steps > 0){
            for(int i=1; i<=steps; i++){
                cout << "idx+i: " << idx+i << endl;
                if (get_original_idx(idx) + steps < get_original_idx(idx+i)) {
                    cout << "breaking, original idx: " << get_original_idx(idx) << " steps: " << steps << " dest: " << get_original_idx(idx+i) << endl;
                    break;
                }
                if(idx+i < size) {
                    std::cout << "A min: " << min << " dp[idx+1]: " << dp[idx+i] << endl;
                    if (dp[idx+i] <= min) {
                        min = dp[idx+i];
                        min_idx = idx+i;
                    }
                    std::cout << "B min: " << min << " min_idx: " << min_idx << endl;
                }
            }
        }
        bool can_reach_end = arr[idx] >= get_diff(idx, min_idx);
        cout << "can reach end: " << can_reach_end << " arr[idx]: " << arr[idx] << " diff: " << get_diff(idx, min_idx) <<  endl;
        if (can_reach_end)
            dp[idx] = min == INT_MAX ? min : min+1;
        else
            dp[idx] = 0;
        cout << "cal dp[idx]: " << dp[idx] << endl;
    }
}

int main() {
    int size = 6;
    int arr[size] = {20,30,10,20,10,0};
    //int arr[size] = {3,2,10,9,3,1,1,4};
    int dp[size] = {0};
    
    minJumps(dp, arr, size);
    
    cout << endl << endl << "dp: ";
    for (int i : dp) {
        cout << i << " ";
    }
    cout << "\n\n";
    
    get_path(dp, arr, size);

    return 0;
}
