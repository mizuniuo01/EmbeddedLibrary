#ifndef MIZU_MENU_SERVICE_H
#define MIZU_MENU_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "command.h"
#include "mizu_status.h"
#include "parameter.h"

typedef uint16_t mizu_menu_group_id_t;
typedef uint16_t mizu_menu_page_id_t;

typedef struct {
    const char *data;
    size_t length;
} mizu_menu_text_t;

typedef enum { MIZU_MENU_ITEM_PARAMETER = 0, MIZU_MENU_ITEM_COMMAND } mizu_menu_item_kind_t;

typedef struct {
    mizu_menu_item_kind_t kind;
    union {
        mizu_parameter_id_t parameter_id;
        struct {
            mizu_command_id_t command_id;
            bool requires_confirmation;
        } command;
    } target;
} mizu_menu_item_t;

typedef struct {
    mizu_menu_group_id_t id;
    mizu_menu_text_t title;
    const mizu_menu_item_t *items;
    size_t item_count;
} mizu_menu_group_t;

typedef struct {
    mizu_menu_page_id_t id;
    mizu_menu_text_t title;
    size_t page_count;
} mizu_menu_display_page_t;

typedef enum { MIZU_MENU_ROOT_GROUP = 0, MIZU_MENU_ROOT_DISPLAY_PAGE } mizu_menu_root_kind_t;

typedef struct {
    mizu_menu_root_kind_t kind;
    union {
        const mizu_menu_group_t *group;
        const mizu_menu_display_page_t *display_page;
    } target;
} mizu_menu_root_item_t;

typedef enum {
    MIZU_MENU_EVENT_MOVE_SELECTION = 0,
    MIZU_MENU_EVENT_MOVE_PAGE,
    MIZU_MENU_EVENT_ACTIVATE,
    MIZU_MENU_EVENT_BACK,
    MIZU_MENU_EVENT_ADJUST_VALUE,
    MIZU_MENU_EVENT_OPEN_STEP_EDITOR,
    MIZU_MENU_EVENT_ADJUST_STEP
} mizu_menu_event_kind_t;

typedef struct {
    mizu_menu_event_kind_t kind;
    int32_t amount;
} mizu_menu_event_t;

typedef enum {
    MIZU_MENU_STATE_ROOT = 0,
    MIZU_MENU_STATE_GROUP,
    MIZU_MENU_STATE_PARAMETER_EDIT,
    MIZU_MENU_STATE_STEP_EDIT,
    MIZU_MENU_STATE_COMMAND_CONFIRM,
    MIZU_MENU_STATE_DISPLAY_PAGE
} mizu_menu_state_t;

typedef enum {
    MIZU_MENU_COMPONENT_ROOT_ITEM = 0,
    MIZU_MENU_COMPONENT_PARAMETER,
    MIZU_MENU_COMPONENT_COMMAND,
    MIZU_MENU_COMPONENT_PARAMETER_EDITOR,
    MIZU_MENU_COMPONENT_COMMAND_CONFIRMATION
} mizu_menu_component_kind_t;

typedef struct {
    bool selected;
    bool editable;
    bool editing;
    bool requires_confirmation;
    bool value_valid;
} mizu_menu_component_flags_t;

typedef struct {
    mizu_menu_component_kind_t kind;
    uint16_t id;
    mizu_menu_text_t label;
    mizu_menu_component_flags_t flags;
    union {
        struct {
            mizu_parameter_value_t value;
            mizu_parameter_value_t minimum;
            mizu_parameter_value_t maximum;
            mizu_parameter_value_t default_step;
            mizu_parameter_value_t current_step;
            mizu_menu_text_t unit;
            uint8_t decimals;
            mizu_status_t value_status;
        } parameter;
    } data;
} mizu_menu_component_t;

typedef enum {
    MIZU_MENU_FEEDBACK_NONE = 0,
    MIZU_MENU_FEEDBACK_PARAMETER,
    MIZU_MENU_FEEDBACK_COMMAND
} mizu_menu_feedback_kind_t;

typedef enum {
    MIZU_MENU_FEEDBACK_RESULT = 0,
    MIZU_MENU_FEEDBACK_APPLIED,
    MIZU_MENU_FEEDBACK_CONFLICT,
    MIZU_MENU_FEEDBACK_READ_FAILED,
    MIZU_MENU_FEEDBACK_WRITE_FAILED
} mizu_menu_feedback_reason_t;

typedef struct {
    mizu_menu_feedback_kind_t kind;
    mizu_menu_feedback_reason_t reason;
    uint16_t id;
    mizu_menu_text_t label;
    mizu_status_t status;
    bool active;
} mizu_menu_feedback_t;

typedef struct {
    mizu_menu_state_t state;
    mizu_menu_text_t title;
    mizu_menu_page_id_t display_page_id;
    size_t component_count;
    size_t total_count;
    size_t page_index;
    size_t page_count;
    size_t display_subpage_index;
    size_t display_subpage_count;
    mizu_menu_feedback_t feedback;
} mizu_menu_view_t;

typedef struct {
    mizu_menu_text_t root_title;
    const mizu_menu_root_item_t *root_items;
    size_t root_item_count;
    mizu_parameter_service_t *parameters;
    mizu_command_service_t *commands;
    mizu_parameter_value_t *step_cache;
    size_t step_cache_count;
    uint32_t feedback_duration_ms;
} mizu_menu_config_t;

/* 调用方拥有实例、描述表和步长缓存；字段只能通过公共 API 操作。 */
typedef struct {
    mizu_menu_config_t config;
    mizu_parameter_value_t edit_value;
    mizu_parameter_value_t original_value;
    mizu_parameter_value_t original_step;
    mizu_menu_feedback_t feedback;
    mizu_menu_state_t state;
    size_t root_index;
    size_t item_index;
    size_t display_subpage_index;
    size_t step_cache_index;
    uint32_t feedback_deadline_ms;
    bool initialized;
} mizu_menu_service_t;

/**
 * mizu_menu_init - 验证并绑定静态菜单描述和服务依赖。
 * @menu: 调用方拥有的菜单实例，不要求预先清零。
 * @config: 静态描述、服务依赖、步长缓存和反馈时长配置。
 *
 * 只在装配阶段调用一次。菜单借用所有指针，不分配或释放资源；描述表、文本、服务和缓存
 * 必须在菜单生命周期内保持有效。步长缓存元素数必须等于参数服务注册参数总数。所有菜单
 * 接口只能在同一主循环或已同步的普通任务上下文串行调用，不支持 ISR、并发或重入。
 * 返回值：成功返回 MIZU_STATUS_OK；结构或绑定非法返回 MIZU_STATUS_INVALID_ARGUMENT；依赖服务
 * 未初始化返回 MIZU_STATUS_NOT_INITIALIZED；ID 未注册或缓存数量错误返回
 * MIZU_STATUS_OUT_OF_RANGE。失败时菜单对象和步长缓存保持不变。
 */
mizu_status_t mizu_menu_init(mizu_menu_service_t *menu, const mizu_menu_config_t *config);

/**
 * mizu_menu_handle_event - 处理一个已适配的同步逻辑输入事件。
 * @menu: 已初始化的菜单实例。
 * @event: 输入适配器生成的事件；原始按键时序不得传入本接口。
 * @now_ms: 当前可回绕单调毫秒时间。
 *
 * 支持根/组导航、参数草稿与步长编辑、命令确认与单次执行、展示页子页导航。
 * MOVE_SELECTION、MOVE_PAGE 和 ADJUST_VALUE 使用有符号 amount；ADJUST_STEP 的 amount 只允许
 * -1、0 或 1，其他值返回 MIZU_STATUS_INVALID_ARGUMENT。ACTIVATE、BACK 和 OPEN_STEP_EDITOR
 * 忽略 amount。参数 getter/setter 的业务结果通过 feedback 保存；事件被正确消费时本函数返回
 * MIZU_STATUS_OK。空指针返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；当前状态不接受该事件返回 MIZU_STATUS_INVALID_STATE；依赖服务
 * 的描述符错误原样传播。函数非阻塞，失败时菜单导航和草稿状态保持不变。
 */
mizu_status_t mizu_menu_handle_event(mizu_menu_service_t *menu, const mizu_menu_event_t *event,
                                     uint32_t now_ms);

/**
 * mizu_menu_process - 推进菜单的非阻塞限时状态。
 * @menu: 已初始化的菜单实例。
 * @now_ms: 当前可回绕单调毫秒时间。
 *
 * 当前仅清除到期 feedback，不读取输入或外部数据。返回值：成功返回 MIZU_STATUS_OK；空指针
 * 返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回 MIZU_STATUS_NOT_INITIALIZED。
 */
mizu_status_t mizu_menu_process(mizu_menu_service_t *menu, uint32_t now_ms);

/**
 * mizu_menu_query_view - 查询当前导航上下文的一页语义组件。
 * @menu: 已初始化的菜单实例。
 * @visible_count: 本次输出适配器可显示的条目数量。
 * @components: 输出适配器提供的固定组件数组。
 * @component_capacity: @components 的元素容量。
 * @view: 接收页面元数据、分页信息和 feedback 的输出地址。
 *
 * 查询只读取菜单状态和 parameter 当前值，不执行命令，不修改菜单状态。组件数组由调用方
 * 提供并由调用方持有；基础参数错误或菜单未初始化时不承诺输出。容量不足或描述符预检失败
 * 时，view 提供当前页面元数据和所需组件数量，components 不写入；只有返回 OK 时组件数组
 * 有效。parameter getter 失败只记录在对应组件的 value_status 中并返回 OK。展示页返回导航
 * 元数据，组件数量为零，数据由外部展示适配器读取和组合。
 */
mizu_status_t mizu_menu_query_view(const mizu_menu_service_t *menu, size_t visible_count,
                                   mizu_menu_component_t *components, size_t component_capacity,
                                   mizu_menu_view_t *view);

#endif /* MIZU_MENU_SERVICE_H */
