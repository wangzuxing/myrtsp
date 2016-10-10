 #ifndef _DOUBLE_LINK_H
 2#define _DOUBLE_LINK_H
 3
 4 // �½���˫���������ɹ������ر�ͷ�����򣬷���NULL
 5 extern int create_dlink();
 6 // ������˫���������ɹ�������0�����򣬷���-1
 7 extern int destroy_dlink();
 8
 9 // ��˫�������Ƿ�Ϊ�ա���Ϊ�յĻ�����1�����򣬷���0��
10 extern int dlink_is_empty();
11 // ���ء�˫������Ĵ�С��
12 extern int dlink_size();
13
14 // ��ȡ��˫�������е�indexλ�õ�Ԫ�ء����ɹ������ؽڵ�ָ�룻���򣬷���NULL��
15 extern void* dlink_get(int index);
16 // ��ȡ��˫�������е�1��Ԫ�ء����ɹ������ؽڵ�ָ�룻���򣬷���NULL��
17 extern void* dlink_get_first();
18 // ��ȡ��˫�����������1��Ԫ�ء����ɹ������ؽڵ�ָ�룻���򣬷���NULL��
19 extern void* dlink_get_last();
20
21 // ����value�����뵽indexλ�á��ɹ�������0�����򣬷���-1��
22 extern int dlink_insert(int index, void *pval);
23 // ����value�����뵽��ͷλ�á��ɹ�������0�����򣬷���-1��
24 extern int dlink_insert_first(void *pval);
25 // ����value�����뵽ĩβλ�á��ɹ�������0�����򣬷���-1��
26 extern int dlink_append_last(void *pval);
27
28 // ɾ����˫��������indexλ�õĽڵ㡱���ɹ�������0�����򣬷���-1
29 extern int dlink_delete(int index);
30 // ɾ����һ���ڵ㡣�ɹ�������0�����򣬷���-1
31 extern int dlink_delete_first();
32 // ɾ�����һ���ڵ㡣�ɹ�������0�����򣬷���-1
33 extern int dlink_delete_last();
34
35 #endif
