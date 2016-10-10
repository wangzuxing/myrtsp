 #include <stdio.h>
  2 #include <malloc.h>
  3
  4 /**
  5  * C 语言: 双向链表，能存储任意数据。
  6  *
  7  * @author skywang
  8  * @date 2013/11/07
  9  */
 10
 11 // 双向链表节点
 12 typedef struct tag_node
 13 {
 14     struct tag_node *prev;
 15     struct tag_node *next;
 16     void* p;
 17 }node;
 18
 19 // 表头。注意，表头不存放元素值！！！
 20 static node *phead=NULL;
 21 // 节点个数。
 22 static int  count=0;
 23
 24 // 新建“节点”。成功，返回节点指针；否则，返回NULL。
 25 static node* create_node(void *pval)
 26 {
 27     node *pnode=NULL;
 28     pnode = (node *)malloc(sizeof(node));
 29     if (!pnode)
 30     {
 31         printf("create node error!\n");
 32         return NULL;
 33     }
 34     // 默认的，pnode的前一节点和后一节点都指向它自身
 35     pnode->prev = pnode->next = pnode;
 36     // 节点的值为pval
 37     pnode->p = pval;
 38
 39     return pnode;
 40 }
 41
 42 // 新建“双向链表”。成功，返回0；否则，返回-1。
 43 int create_dlink()
 44 {
 45     // 创建表头
 46     phead = create_node(NULL);
 47     if (!phead)
 48         return -1;
 49
 50     // 设置“节点个数”为0
 51     count = 0;
 52
 53     return 0;
 54 }
 55
 56 // “双向链表是否为空”
 57 int dlink_is_empty()
 58 {
 59     return count == 0;
 60 }
 61
 62 // 返回“双向链表的大小”
 63 int dlink_size() {
 64     return count;
 65 }
 66
 67 // 获取“双向链表中第index位置的节点”
 68 static node* get_node(int index)
 69 {
 70     if (index<0 || index>=count)
 71     {
 72         printf("%s failed! index out of bound!\n", __func__);
 73         return NULL;
 74     }
 75
 76     // 正向查找
 77     if (index <= (count/2))
 78     {
 79         int i=0;
 80         node *pnode=phead->next;
 81         while ((i++) < index)
 82             pnode = pnode->next;
 83
 84         return pnode;
 85     }
 86
 87     // 反向查找
 88     int j=0;
 89     int rindex = count - index - 1;
 90     node *rnode=phead->prev;
 91     while ((j++) < rindex)
 92         rnode = rnode->prev;
 93
 94     return rnode;
 95 }
 96
 97 // 获取“第一个节点”
 98 static node* get_first_node()
 99 {
100     return get_node(0);
101 }
102
103 // 获取“最后一个节点”
104 static node* get_last_node()
105 {
106     return get_node(count-1);
107 }
108
109 // 获取“双向链表中第index位置的元素”。成功，返回节点值；否则，返回-1。
110 void* dlink_get(int index)
111 {
112     node *pindex=get_node(index);
113     if (!pindex)
114     {
115         printf("%s failed!\n", __func__);
116         return NULL;
117     }
118
119     return pindex->p;
120
121 }
122
123 // 获取“双向链表中第1个元素的值”
124 void* dlink_get_first()
125 {
126     return dlink_get(0);
127 }
128
129 // 获取“双向链表中最后1个元素的值”
130 void* dlink_get_last()
131 {
132     return dlink_get(count-1);
133 }
134
135 // 将“pval”插入到index位置。成功，返回0；否则，返回-1。
136 int dlink_insert(int index, void* pval)
137 {
138     // 插入表头
139     if (index==0)
140         return dlink_insert_first(pval);
141
142     // 获取要插入的位置对应的节点
143     node *pindex=get_node(index);
144     if (!pindex)
145         return -1;
146
147     // 创建“节点”
148     node *pnode=create_node(pval);
149     if (!pnode)
150         return -1;
151
152     pnode->prev = pindex->prev;
153     pnode->next = pindex;
154     pindex->prev->next = pnode;
155     pindex->prev = pnode;
156     // 节点个数+1
157     count++;
158
159     return 0;
160 }
161
162 // 将“pval”插入到表头位置
163 int dlink_insert_first(void *pval)
164 {
165     node *pnode=create_node(pval);
166     if (!pnode)
167         return -1;
168
169     pnode->prev = phead;
170     pnode->next = phead->next;
171     phead->next->prev = pnode;
172     phead->next = pnode;
173     count++;
174     return 0;
175 }
176
177 // 将“pval”插入到末尾位置
178 int dlink_append_last(void *pval)
179 {
180     node *pnode=create_node(pval);
181     if (!pnode)
182         return -1;
183
184     pnode->next = phead;
185     pnode->prev = phead->prev;
186     phead->prev->next = pnode;
187     phead->prev = pnode;
188     count++;
189     return 0;
190 }
191
192 // 删除“双向链表中index位置的节点”。成功，返回0；否则，返回-1。
193 int dlink_delete(int index)
194 {
195     node *pindex=get_node(index);
196     if (!pindex)
197     {
198         printf("%s failed! the index in out of bound!\n", __func__);
199         return -1;
200     }
201
202     pindex->next->prev = pindex->prev;
203     pindex->prev->next = pindex->next;
204     free(pindex);
205     count--;
206
207     return 0;
208 }
209
210 // 删除第一个节点
211 int dlink_delete_first()
212 {
213     return dlink_delete(0);
214 }
215
216 // 删除组后一个节点
217 int dlink_delete_last()
218 {
219     return dlink_delete(count-1);
220 }
221
222 // 撤销“双向链表”。成功，返回0；否则，返回-1。
223 int destroy_dlink()
224 {
225     if (!phead)
226     {
227         printf("%s failed! dlink is null!\n", __func__);
228         return -1;
229     }
230
231     node *pnode=phead->next;
232     node *ptmp=NULL;
233     while(pnode != phead)
234     {
235         ptmp = pnode;
236         pnode = pnode->next;
237         free(ptmp);
238     }
239
240     free(phead);
241     phead = NULL;
242     count = 0;
243
244     return 0;
245 }

/**
 #include<stdio.h>
typedef struct dataNode
{
    int data;
    struct dataNode *next;
}qNode;

typedef struct linkQueue
{
    qNode *head;
    qNode *tail;
}linkQueue;

//创建一个新队列
linkQueue* createLinkQueue()
{
    linkQueue *q = (linkQueue*)malloc(sizeof(linkQueue));
    q->head = NULL;
    q->tail = NULL;
    return q;
}

//向队列插入一个结点
linkQueue* insertNode(linkQueue *q,int i)
{
    qNode *node;
    node = (qNode*)malloc(sizeof(qNode));
    node ->data = i;
    node ->next = NULL;
    if(q->tail == NULL)
    {
        q->head = node;
        q->tail = node;
    }
    else
    {
        q->tail->next = node;
        q->tail = node;
    }
    return q;
}

//从队列删除一个结点
linkQueue* deleteNode(linkQueue *q)
{
    qNode *node;
    if(q->head == NULL)
    {
        printf("the queue is empty \n");
        return q;
    }

    node = q->head;
    if(q->head == q->tail)
    {
        q->head = NULL;
        q->tail = NULL;
        free(node);
    }else
    {
        q->head = q->head->next;
        free(node);
    }
    return q;
}

//顺序打印队列
void printLinkQueue(linkQueue *q)
{
    qNode *node;
    node = q->head;
    while(node)
    {
        printf("%d  ",node->data);
        node=node->next;
    }
    printf("\n");
}
 * /
