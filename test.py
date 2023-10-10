import clang.cindex

index = clang.cindex.Index.create()
tu = index.parse("framework/include/bt_device.h", args=['-std=c99'])

for node in tu.cursor.walk_preorder():
  if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
    print('struct ' + node.spelling + '_s')
    print('{')
    if node.result_type.spelling == 'void *':
      print('  ' + node.result_type.spelling + ' __ret;')
    elif node.result_type.spelling != 'void':
      print('  ' + node.result_type.spelling.replace("*","") + ' __ret;')
    for arg in node.get_arguments():
        print('  ' + arg.type.spelling.replace("*","") + ' ' + arg.spelling + ';')
    print('};\n')

    print(arg.type.spelling + ' ' + node.spelling + '(', end="")
    first = True
    for arg in node.get_arguments():
        if first != True:
          print(', ', end="")
        first = False
        print(arg.type.spelling + ' ' + arg.spelling, end="")
    print(')')
    print('{')
    print('}\n')

for node in tu.cursor.walk_preorder():
  if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
    print(node.spelling.upper() + ',')
