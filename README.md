# ZnSerialize

做这个工具的目的就是为了解决我项目上网络传输时的数据序列化与反序列化的问题
用过google的protobuf，用着太麻烦了，而且我的简单需求根本配不上那么大的一个库。
今天突然有个思路用可变参模板实现一个通用的好像很简单，就卷起袖子撸了一下，发现200多行就实现了功能。
整个代码简单，因为都是用的基本语法与标准库，所以也没有平台限制
要是需要什么自定义的格式自己特化模板就行了。

2022-01-05 增加对数据结构继承的支持
