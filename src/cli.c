#include "cli.h"

#include "input.h"
#include "repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_CAPACITY 256

static void print_usage(const char *program) {
    printf("实验室仪器信息管理工具\n\n");
    printf("用法:\n");
    printf("  %s [--data <file>]                 启动交互界面\n", program);
    printf("  %s [--data <file>] list            列出全部仪器\n", program);
    printf("  %s [--data <file>] show <id>       按编号查看\n", program);
    printf("  %s [--data <file>] search <text>   搜索仪器\n", program);
    printf("  %s [--data <file>] stats           显示库存统计\n", program);
    printf("  %s [--data <file>] add <name> <category> <spec> <model> <date> <price> <qty>\n", program);
    printf("  %s [--data <file>] update <id> <field> <value>\n", program);
    printf("  %s [--data <file>] remove <id>\n", program);
    printf("\nupdate 字段: name, category, specification, model, date, price, quantity\n");
    printf("日期格式: YYYY-MM-DD；包含空格的参数需要使用引号。\n");
}

static void print_instrument(const Instrument *instrument) {
    if (instrument == NULL) {
        return;
    }
    printf("编号: %d\n", instrument->id);
    printf("名称: %s\n", instrument->name);
    printf("类别: %s\n", instrument->category);
    printf("规格: %s\n", instrument->specification);
    printf("型号: %s\n", instrument->model);
    printf("购入日期: %s\n", instrument->purchase_date);
    printf("单价: %.2f\n", instrument->price);
    printf("数量: %d\n", instrument->quantity);
    printf("库存金额: %.2f\n", instrument->price * instrument->quantity);
}

static void print_list(const InstrumentRepository *repository) {
    size_t index;
    if (repository->size == 0) {
        printf("暂无仪器数据。\n");
        return;
    }
    printf("%-5s | %-20s | %-16s | %-12s | %-12s | %-10s | %10s | %6s\n",
           "ID", "NAME", "CATEGORY", "SPEC", "MODEL", "DATE", "PRICE", "QTY");
    printf("--------------------------------------------------------------------------------------------------------------\n");
    for (index = 0; index < repository->size; ++index) {
        const Instrument *item = &repository->items[index];
        printf("%-5d | %-20.20s | %-16.16s | %-12.12s | %-12.12s | %-10.10s | %10.2f | %6d\n",
               item->id, item->name, item->category, item->specification, item->model,
               item->purchase_date, item->price, item->quantity);
    }
    printf("共 %lu 条记录。\n", (unsigned long)repository->size);
}

static void print_search(const InstrumentRepository *repository, const char *keyword) {
    size_t index;
    size_t matches = 0;
    for (index = 0; index < repository->size; ++index) {
        if (instrument_contains(&repository->items[index], keyword)) {
            if (matches > 0) {
                printf("------------------------------\n");
            }
            print_instrument(&repository->items[index]);
            ++matches;
        }
    }
    if (matches == 0) {
        printf("没有找到包含“%s”的仪器。\n", keyword);
    } else {
        printf("找到 %lu 条记录。\n", (unsigned long)matches);
    }
}

static void print_stats(const InstrumentRepository *repository) {
    size_t index;
    size_t previous;
    size_t categories = 0;
    long total_quantity = 0;
    size_t low_stock = 0;
    double total_value = 0.0;
    const Instrument *highest = NULL;
    for (index = 0; index < repository->size; ++index) {
        const Instrument *item = &repository->items[index];
        int first_category = 1;
        total_quantity += item->quantity;
        total_value += item->price * item->quantity;
        if (item->quantity < 10) {
            ++low_stock;
        }
        if (highest == NULL || item->price * item->quantity > highest->price * highest->quantity) {
            highest = item;
        }
        for (previous = 0; previous < index; ++previous) {
            if (strcmp(item->category, repository->items[previous].category) == 0) {
                first_category = 0;
                break;
            }
        }
        if (first_category) {
            ++categories;
        }
    }
    printf("仪器种类数: %lu\n", (unsigned long)repository->size);
    printf("类别数: %lu\n", (unsigned long)categories);
    printf("库存总件数: %ld\n", total_quantity);
    printf("库存总金额: %.2f\n", total_value);
    printf("低库存条目（数量 < 10）: %lu\n", (unsigned long)low_stock);
    if (highest != NULL) {
        printf("库存金额最高: %s (%.2f)\n", highest->name,
               highest->price * highest->quantity);
    }
}

static int copy_text(char *destination, size_t capacity, const char *source,
                     char *error, size_t error_size) {
    if (source == NULL || strlen(source) >= capacity) {
        snprintf(error, error_size, "文本不能为空且长度必须小于 %lu 字节",
                 (unsigned long)capacity);
        return 0;
    }
    snprintf(destination, capacity, "%s", source);
    return 1;
}

static int build_instrument_from_arguments(InstrumentRepository *repository, char **arguments,
                                           Instrument *instrument, char *error, size_t error_size) {
    memset(instrument, 0, sizeof(*instrument));
    instrument->id = repository_next_id(repository);
    if (!copy_text(instrument->name, sizeof(instrument->name), arguments[0], error, error_size)
        || !copy_text(instrument->category, sizeof(instrument->category), arguments[1], error, error_size)
        || !copy_text(instrument->specification, sizeof(instrument->specification), arguments[2], error, error_size)
        || !copy_text(instrument->model, sizeof(instrument->model), arguments[3], error, error_size)
        || !copy_text(instrument->purchase_date, sizeof(instrument->purchase_date), arguments[4], error, error_size)
        || !input_parse_double(arguments[5], &instrument->price)
        || !input_parse_int(arguments[6], &instrument->quantity)) {
        if (*error == '\0') {
            snprintf(error, error_size, "price 或 quantity 格式错误");
        }
        return 0;
    }
    return instrument_validate(instrument, error, error_size);
}

static int apply_update(Instrument *instrument, const char *field, const char *value,
                        char *error, size_t error_size) {
    if (strcmp(field, "name") == 0) {
        return copy_text(instrument->name, sizeof(instrument->name), value, error, error_size);
    }
    if (strcmp(field, "category") == 0) {
        return copy_text(instrument->category, sizeof(instrument->category), value, error, error_size);
    }
    if (strcmp(field, "specification") == 0 || strcmp(field, "spec") == 0) {
        return copy_text(instrument->specification, sizeof(instrument->specification), value, error, error_size);
    }
    if (strcmp(field, "model") == 0) {
        return copy_text(instrument->model, sizeof(instrument->model), value, error, error_size);
    }
    if (strcmp(field, "date") == 0 || strcmp(field, "purchase_date") == 0) {
        return copy_text(instrument->purchase_date, sizeof(instrument->purchase_date), value, error, error_size);
    }
    if (strcmp(field, "price") == 0) {
        if (!input_parse_double(value, &instrument->price)) {
            snprintf(error, error_size, "price 格式错误");
            return 0;
        }
        return 1;
    }
    if (strcmp(field, "quantity") == 0 || strcmp(field, "qty") == 0) {
        if (!input_parse_int(value, &instrument->quantity)) {
            snprintf(error, error_size, "quantity 格式错误");
            return 0;
        }
        return 1;
    }
    snprintf(error, error_size, "未知字段: %s", field);
    return 0;
}

static int save_or_report(const InstrumentRepository *repository, const char *path) {
    char error[ERROR_CAPACITY] = "";
    if (!repository_save(repository, path, error, sizeof(error))) {
        fprintf(stderr, "保存失败: %s\n", error);
        return 0;
    }
    return 1;
}

static int prompt_text(const char *prompt, char *destination, size_t capacity) {
    while (input_read_line(prompt, destination, capacity)) {
        if (*destination != '\0') {
            return 1;
        }
        printf("内容不能为空。\n");
    }
    return 0;
}

static void interactive_add(InstrumentRepository *repository, const char *path) {
    Instrument instrument;
    char error[ERROR_CAPACITY] = "";
    memset(&instrument, 0, sizeof(instrument));
    instrument.id = repository_next_id(repository);
    if (!prompt_text("名称: ", instrument.name, sizeof(instrument.name))
        || !prompt_text("类别: ", instrument.category, sizeof(instrument.category))
        || !prompt_text("规格: ", instrument.specification, sizeof(instrument.specification))
        || !prompt_text("型号: ", instrument.model, sizeof(instrument.model))
        || !prompt_text("购入日期 (YYYY-MM-DD): ", instrument.purchase_date, sizeof(instrument.purchase_date))
        || !input_prompt_double("单价: ", 0.0, &instrument.price)
        || !input_prompt_int("数量: ", 0, &instrument.quantity)) {
        printf("已取消。\n");
        return;
    }
    if (!repository_add(repository, instrument, error, sizeof(error))) {
        printf("新增失败: %s\n", error);
        return;
    }
    if (!save_or_report(repository, path)) {
        repository_remove(repository, instrument.id, NULL, 0);
        return;
    }
    printf("新增成功，编号为 %d。\n", instrument.id);
}

static void interactive_update(InstrumentRepository *repository, const char *path) {
    int id;
    char field[64];
    char value[INSTRUMENT_TEXT_CAPACITY];
    char error[ERROR_CAPACITY] = "";
    Instrument *existing;
    Instrument original;
    Instrument updated;
    if (!input_prompt_int("要修改的编号: ", 1, &id)) {
        return;
    }
    existing = repository_find_mutable_by_id(repository, id);
    if (existing == NULL) {
        printf("未找到编号 %d。\n", id);
        return;
    }
    print_instrument(existing);
    if (!prompt_text("字段 (name/category/specification/model/date/price/quantity): ", field, sizeof(field))
        || !prompt_text("新值: ", value, sizeof(value))) {
        return;
    }
    original = *existing;
    updated = original;
    if (!apply_update(&updated, field, value, error, sizeof(error))
        || !repository_update(repository, updated, error, sizeof(error))) {
        printf("修改失败: %s\n", error);
        return;
    }
    if (!save_or_report(repository, path)) {
        *existing = original;
        return;
    }
    printf("修改成功。\n");
}

static void interactive_remove(InstrumentRepository *repository, const char *path) {
    int id;
    char confirmation[16];
    char error[ERROR_CAPACITY] = "";
    const Instrument *existing;
    Instrument removed;
    if (!input_prompt_int("要删除的编号: ", 1, &id)) {
        return;
    }
    existing = repository_find_by_id(repository, id);
    if (existing == NULL) {
        printf("未找到编号 %d。\n", id);
        return;
    }
    removed = *existing;
    print_instrument(existing);
    if (!input_read_line("确认删除？输入 yes: ", confirmation, sizeof(confirmation))
        || strcmp(confirmation, "yes") != 0) {
        printf("已取消。\n");
        return;
    }
    if (!repository_remove(repository, id, error, sizeof(error))) {
        printf("删除失败: %s\n", error);
        return;
    }
    if (!save_or_report(repository, path)) {
        repository_add(repository, removed, NULL, 0);
        return;
    }
    printf("删除成功。\n");
}

static int interactive_loop(InstrumentRepository *repository, const char *path) {
    char choice[32];
    while (1) {
        printf("\n========== 实验室仪器信息管理 ==========\n");
        printf("1. 查看全部    2. 按编号查看    3. 搜索\n");
        printf("4. 新增        5. 修改          6. 删除\n");
        printf("7. 库存统计    0. 退出\n");
        if (!input_read_line("请选择: ", choice, sizeof(choice))) {
            printf("\n输入结束。\n");
            return 0;
        }
        if (strcmp(choice, "1") == 0) {
            print_list(repository);
        } else if (strcmp(choice, "2") == 0) {
            int id;
            const Instrument *item;
            if (input_prompt_int("编号: ", 1, &id)) {
                item = repository_find_by_id(repository, id);
                if (item == NULL) {
                    printf("未找到编号 %d。\n", id);
                } else {
                    print_instrument(item);
                }
            }
        } else if (strcmp(choice, "3") == 0) {
            char keyword[INSTRUMENT_TEXT_CAPACITY];
            if (prompt_text("关键字: ", keyword, sizeof(keyword))) {
                print_search(repository, keyword);
            }
        } else if (strcmp(choice, "4") == 0) {
            interactive_add(repository, path);
        } else if (strcmp(choice, "5") == 0) {
            interactive_update(repository, path);
        } else if (strcmp(choice, "6") == 0) {
            interactive_remove(repository, path);
        } else if (strcmp(choice, "7") == 0) {
            print_stats(repository);
        } else if (strcmp(choice, "0") == 0) {
            return 0;
        } else {
            printf("无效选项，请输入 0-7。\n");
        }
    }
}

static int run_command(InstrumentRepository *repository, const char *path,
                       int count, char **arguments) {
    const char *command = arguments[0];
    char error[ERROR_CAPACITY] = "";
    if (strcmp(command, "list") == 0 && count == 1) {
        print_list(repository);
        return 0;
    }
    if (strcmp(command, "stats") == 0 && count == 1) {
        print_stats(repository);
        return 0;
    }
    if (strcmp(command, "show") == 0 && count == 2) {
        int id;
        const Instrument *item;
        if (!input_parse_int(arguments[1], &id) || id <= 0) {
            fprintf(stderr, "编号格式错误。\n");
            return 2;
        }
        item = repository_find_by_id(repository, id);
        if (item == NULL) {
            fprintf(stderr, "未找到编号 %d。\n", id);
            return 1;
        }
        print_instrument(item);
        return 0;
    }
    if (strcmp(command, "search") == 0 && count == 2) {
        print_search(repository, arguments[1]);
        return 0;
    }
    if (strcmp(command, "add") == 0 && count == 8) {
        Instrument instrument;
        if (!build_instrument_from_arguments(repository, &arguments[1], &instrument,
                                             error, sizeof(error))
            || !repository_add(repository, instrument, error, sizeof(error))) {
            fprintf(stderr, "新增失败: %s\n", error);
            return 1;
        }
        if (!save_or_report(repository, path)) {
            repository_remove(repository, instrument.id, NULL, 0);
            return 1;
        }
        printf("新增成功，编号为 %d。\n", instrument.id);
        return 0;
    }
    if (strcmp(command, "update") == 0 && count == 4) {
        int id;
        Instrument *existing;
        Instrument updated;
        if (!input_parse_int(arguments[1], &id) || id <= 0
            || (existing = repository_find_mutable_by_id(repository, id)) == NULL) {
            fprintf(stderr, "未找到有效编号。\n");
            return 1;
        }
        updated = *existing;
        if (!apply_update(&updated, arguments[2], arguments[3], error, sizeof(error))
            || !repository_update(repository, updated, error, sizeof(error))) {
            fprintf(stderr, "修改失败: %s\n", error);
            return 1;
        }
        if (!save_or_report(repository, path)) {
            return 1;
        }
        printf("修改成功。\n");
        return 0;
    }
    if (strcmp(command, "remove") == 0 && count == 2) {
        int id;
        if (!input_parse_int(arguments[1], &id)
            || !repository_remove(repository, id, error, sizeof(error))) {
            fprintf(stderr, "删除失败: %s\n", *error == '\0' ? "编号格式错误" : error);
            return 1;
        }
        if (!save_or_report(repository, path)) {
            return 1;
        }
        printf("删除成功。\n");
        return 0;
    }
    if ((strcmp(command, "help") == 0 || strcmp(command, "--help") == 0) && count == 1) {
        print_usage("lab-manager");
        return 0;
    }
    fprintf(stderr, "命令或参数不正确。\n\n");
    print_usage("lab-manager");
    return 2;
}

int cli_run(int argc, char **argv) {
    const char *path = "data/instruments.tsv";
    int offset = 1;
    int result;
    char error[ERROR_CAPACITY] = "";
    InstrumentRepository repository;
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "--data") == 0) {
        if (argc < 3) {
            fprintf(stderr, "--data 后必须提供文件路径。\n");
            return 2;
        }
        path = argv[2];
        offset = 3;
    }
    repository_init(&repository);
    if (!repository_load(&repository, path, error, sizeof(error))) {
        fprintf(stderr, "加载失败: %s\n", error);
        repository_free(&repository);
        return 1;
    }
    result = offset >= argc ? interactive_loop(&repository, path)
                            : run_command(&repository, path, argc - offset, &argv[offset]);
    repository_free(&repository);
    return result;
}
