#include "input.h"
#include "repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int assertions = 0;

#define ASSERT_TRUE(condition) do { \
    ++assertions; \
    if (!(condition)) { \
        fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

static Instrument make_instrument(int id, const char *name) {
    Instrument instrument;
    memset(&instrument, 0, sizeof(instrument));
    instrument.id = id;
    snprintf(instrument.name, sizeof(instrument.name), "%s", name);
    snprintf(instrument.category, sizeof(instrument.category), "Optics and Sensors");
    snprintf(instrument.specification, sizeof(instrument.specification), "10 x 20 mm");
    snprintf(instrument.model, sizeof(instrument.model), "A\\B-100");
    snprintf(instrument.purchase_date, sizeof(instrument.purchase_date), "2026-08-03");
    instrument.price = 125.50;
    instrument.quantity = 8;
    return instrument;
}

static void test_validation_and_parsing(void) {
    Instrument valid = make_instrument(1, "Optical Sensor");
    char error[256] = "";
    int integer;
    double number;
    ASSERT_TRUE(instrument_validate(&valid, error, sizeof(error)));
    valid.quantity = -1;
    ASSERT_TRUE(!instrument_validate(&valid, error, sizeof(error)));
    valid = make_instrument(1, "Optical Sensor");
    snprintf(valid.purchase_date, sizeof(valid.purchase_date), "2026/08/03");
    ASSERT_TRUE(!instrument_validate(&valid, error, sizeof(error)));
    ASSERT_TRUE(input_parse_int("42", &integer) && integer == 42);
    ASSERT_TRUE(!input_parse_int("42x", &integer));
    ASSERT_TRUE(input_parse_double("19.75", &number) && number == 19.75);
    ASSERT_TRUE(!input_parse_double("19.75 yuan", &number));
}

static void test_crud(void) {
    InstrumentRepository repository;
    Instrument first = make_instrument(1, "Optical Sensor");
    Instrument second = make_instrument(2, "Digital Multimeter");
    char error[256] = "";
    repository_init(&repository);
    ASSERT_TRUE(repository_add(&repository, first, error, sizeof(error)));
    ASSERT_TRUE(repository_add(&repository, second, error, sizeof(error)));
    ASSERT_TRUE(repository.size == 2);
    ASSERT_TRUE(repository_next_id(&repository) == 3);
    ASSERT_TRUE(!repository_add(&repository, second, error, sizeof(error)));
    first.price = 200.0;
    ASSERT_TRUE(repository_update(&repository, first, error, sizeof(error)));
    ASSERT_TRUE(repository_find_by_id(&repository, 1)->price == 200.0);
    ASSERT_TRUE(instrument_contains(repository_find_by_id(&repository, 2), "multimeter"));
    ASSERT_TRUE(repository_remove(&repository, 1, error, sizeof(error)));
    ASSERT_TRUE(repository_find_by_id(&repository, 1) == NULL);
    ASSERT_TRUE(repository.size == 1);
    ASSERT_TRUE(!repository_remove(&repository, 99, error, sizeof(error)));
    repository_free(&repository);
}

static void test_persistence_and_recovery(void) {
    const char *path = "build/test-data/nested/instruments.tsv";
    const char *backup = "build/test-data/nested/instruments.tsv.bak";
    InstrumentRepository written;
    InstrumentRepository loaded;
    Instrument instrument = make_instrument(7, "Sensor with spaces");
    char error[256] = "";
    remove(path);
    remove(backup);
    repository_init(&written);
    repository_init(&loaded);
    snprintf(instrument.specification, sizeof(instrument.specification), "line one\tline two");
    ASSERT_TRUE(repository_add(&written, instrument, error, sizeof(error)));
    ASSERT_TRUE(repository_save(&written, path, error, sizeof(error)));
    ASSERT_TRUE(repository_load(&loaded, path, error, sizeof(error)));
    ASSERT_TRUE(loaded.size == 1);
    ASSERT_TRUE(strcmp(loaded.items[0].name, "Sensor with spaces") == 0);
    ASSERT_TRUE(strcmp(loaded.items[0].specification, "line one\tline two") == 0);
    ASSERT_TRUE(strcmp(loaded.items[0].model, "A\\B-100") == 0);
    repository_free(&loaded);

    ASSERT_TRUE(rename(path, backup) == 0);
    repository_init(&loaded);
    ASSERT_TRUE(repository_load(&loaded, path, error, sizeof(error)));
    ASSERT_TRUE(loaded.size == 1);
    ASSERT_TRUE(repository_find_by_id(&loaded, 7) != NULL);
    ASSERT_TRUE(remove(path) == 0);
    repository_free(&written);
    repository_free(&loaded);
}

static void test_malformed_data_is_rejected(void) {
    const char *path = "build/test-data/malformed.tsv";
    FILE *file = fopen(path, "wb");
    InstrumentRepository repository;
    char error[256] = "";
    ASSERT_TRUE(file != NULL);
    ASSERT_TRUE(fputs("id\tname\tcategory\tspecification\tmodel\tpurchase_date\tprice\tquantity\n", file) >= 0);
    ASSERT_TRUE(fputs("1\tmissing fields\n", file) >= 0);
    ASSERT_TRUE(fclose(file) == 0);
    repository_init(&repository);
    ASSERT_TRUE(!repository_load(&repository, path, error, sizeof(error)));
    ASSERT_TRUE(strstr(error, "line 2") != NULL);
    repository_free(&repository);
    ASSERT_TRUE(remove(path) == 0);
}

int main(void) {
    test_validation_and_parsing();
    test_crud();
    test_persistence_and_recovery();
    test_malformed_data_is_rejected();
    printf("All tests passed (%d assertions).\n", assertions);
    return 0;
}
