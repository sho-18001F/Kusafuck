#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>


size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int download_and_replace(const char *url, const char *target_filename) {
    CURL *curl;
    CURLcode res;
    FILE *fp;
    

    const char *tmp_filename = "tmp.dat";

    // libcurlの初期化
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "初期化に失敗しました。\n");
        return -1;
    }


    fp = fopen(tmp_filename, "wb");
    if (!fp) {
        perror("一時ファイルの作成に失敗しました");
        curl_easy_cleanup(curl);
        return -1;
    }


    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 

    printf("ダウンロード中: %s\n", url);
    res = curl_easy_perform(curl);


    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "ダウンロード失敗: %s\n", curl_easy_strerror(res));
        remove(tmp_filename); 
        return -1;
    }

    remove(target_filename);
    if (rename(tmp_filename, target_filename) != 0) {
        perror("ファイルの置き換え（リネーム）に失敗しました");
        return -1;
    }

    printf("ファイルの置き換えが完了しました: %s\n", target_filename);
    return 0;
}

int main(void) {

    curl_global_init(CURL_GLOBAL_DEFAULT);


    const char *url = "https://github.com/sho-18001F/Kusafuck/raw/refs/heads/main/";
    const char *target_filename = "kusa.exe";

    download_and_replace(url, target_filename);


    curl_global_cleanup();
    return 0;
}
