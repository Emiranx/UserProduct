#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <sqlite3.h>

#define USERNAME_LENGTH 50
#define PASSWORD_LENGTH 50
#define MAX_USERS 15
#define MAX_PRODUCTS 10

typedef struct {
    char userName[USERNAME_LENGTH];
    char password[PASSWORD_LENGTH];
} User;

typedef struct {
    int productID;
    char productName[USERNAME_LENGTH];
    float price;
} Product;

User users[MAX_USERS];
Product products[MAX_PRODUCTS];
int userCount = 0;
int productCount = 0;

sqlite3* db;

void loadUsersFromJSON(const char* filename) {
    FILE* file;
    if (fopen_s(&file, filename, "r") != 0 || file == NULL) {
        perror("Dosya acilamadi");
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(length + 1);
    if (data == NULL) {
        fclose(file);
        perror("Bellek tahsisi hatasi");
        return;
    }

    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON* json = cJSON_Parse(data);
    if (!json) {
        printf("JSON parse hatasi: %s\n", cJSON_GetErrorPtr());
        free(data);
        return;
    }

    cJSON* usersArray = cJSON_GetObjectItem(json, "users");
    userCount = cJSON_GetArraySize(usersArray);
    for (int i = 0; i < userCount && i < MAX_USERS; i++) {
        cJSON* user = cJSON_GetArrayItem(usersArray, i);
        cJSON* userName = cJSON_GetObjectItem(user, "UserName");
        cJSON* password = cJSON_GetObjectItem(user, "Password");

        if (userName && password) {
            strncpy_s(users[i].userName, USERNAME_LENGTH, userName->valuestring, _TRUNCATE);
            strncpy_s(users[i].password, PASSWORD_LENGTH, password->valuestring, _TRUNCATE);
        }
    }

    cJSON* productsArray = cJSON_GetObjectItem(json, "products");
    productCount = cJSON_GetArraySize(productsArray);
    for (int i = 0; i < productCount && i < MAX_PRODUCTS; i++) {
        cJSON* product = cJSON_GetArrayItem(productsArray, i);
        cJSON* productName = cJSON_GetObjectItem(product, "ProductName");
        cJSON* price = cJSON_GetObjectItem(product, "Price");

        if (productName && price) {
            strncpy_s(products[i].productName, USERNAME_LENGTH, productName->valuestring, _TRUNCATE);
            products[i].price = price->valuedouble;
        }
    }

    cJSON_Delete(json);
    free(data);
}

int checkIfUserExists(const char* username) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM kullanicilar WHERE username = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("Sorgu hazirlama hatasi: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return count > 0;
    }
    else {
        sqlite3_finalize(stmt);
        return 0;
    }
}

int addUserToDatabase(const char* username, const char* password) {
    if (checkIfUserExists(username)) {
        return 0;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO kullanicilar (username, password) VALUES (?, ?);";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 1 : 0;
}

int checkIfProductExists(const char* productName) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM products WHERE UrunAdi = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("Urun kontrol hatasi: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, productName, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return count > 0;
    }
    else {
        sqlite3_finalize(stmt);
        return 0;
    }
}

int addProductToDatabase(const Product* product) {
    if (checkIfProductExists(product->productName)) {
        return 0;
    }

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO products (UrunAdi, Fiyat) VALUES (?, ?);";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, product->productName, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, product->price);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 1 : 0;
}

int authenticate(const char* username, const char* password) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].userName, username) == 0 && strcmp(users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

void displayProducts() {
    printf("Urunler:\n");
    for (int i = 0; i < productCount; i++) {
        printf("Ad: %s, Fiyat: %.2f TL\n", products[i].productName, products[i].price);
    }
}

int main() {
    if (sqlite3_open("proje.db", &db) != SQLITE_OK) {
        printf("Veritabani acilamadi: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_busy_timeout(db, 5000);

    loadUsersFromJSON("UserProduct.js");

    char username[USERNAME_LENGTH];
    char password[PASSWORD_LENGTH];

    printf("Kullanici adi: ");
    scanf_s("%49s", username, (unsigned)_countof(username));
    printf("Sifre: ");
    scanf_s("%49s", password, (unsigned)_countof(password));

    if (authenticate(username, password)) {
        printf("Girisi basarili!\n");

        if (!checkIfUserExists(username)) {
            addUserToDatabase(username, password);  // Hata mesajý vermeden kullanýcý ekleniyor
        }

        for (int i = 0; i < productCount; i++) {
            addProductToDatabase(&products[i]);  // Hata mesajý vermeden ürün ekleniyor
        }

        displayProducts();
    }
    else {
        printf("Giris basarisiz. Lutfen kullanici adinizi ve sifrenizi kontrol edin.\n");
    }

    sqlite3_close(db);
    return 0;
}
