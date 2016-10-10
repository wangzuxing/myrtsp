 #ifndef _DOUBLE_LINK_H
 2#define _DOUBLE_LINK_H
 3
 4 // 新建“双向链表”。成功，返回表头；否则，返回NULL
 5 extern int create_dlink();
 6 // 撤销“双向链表”。成功，返回0；否则，返回-1
 7 extern int destroy_dlink();
 8
 9 // “双向链表是否为空”。为空的话返回1；否则，返回0。
10 extern int dlink_is_empty();
11 // 返回“双向链表的大小”
12 extern int dlink_size();
13
14 // 获取“双向链表中第index位置的元素”。成功，返回节点指针；否则，返回NULL。
15 extern void* dlink_get(int index);
16 // 获取“双向链表中第1个元素”。成功，返回节点指针；否则，返回NULL。
17 extern void* dlink_get_first();
18 // 获取“双向链表中最后1个元素”。成功，返回节点指针；否则，返回NULL。
19 extern void* dlink_get_last();
20
21 // 将“value”插入到index位置。成功，返回0；否则，返回-1。
22 extern int dlink_insert(int index, void *pval);
23 // 将“value”插入到表头位置。成功，返回0；否则，返回-1。
24 extern int dlink_insert_first(void *pval);
25 // 将“value”插入到末尾位置。成功，返回0；否则，返回-1。
26 extern int dlink_append_last(void *pval);
27
28 // 删除“双向链表中index位置的节点”。成功，返回0；否则，返回-1
29 extern int dlink_delete(int index);
30 // 删除第一个节点。成功，返回0；否则，返回-1
31 extern int dlink_delete_first();
32 // 删除组后一个节点。成功，返回0；否则，返回-1
33 extern int dlink_delete_last();
34
35 #endif
