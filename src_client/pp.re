open Ast;

let identifier: type t. Identifier.t(t) => string =
  i =>
    switch (i) {
    | TypeParameter(str) => Printf.sprintf("TypeParameter(%s)", str)
    | Module(str) => Printf.sprintf("Module(%s)", str)
    | TypeName(str) => Printf.sprintf("TypeName(%s)", str)
    | PropertyName(str) => Printf.sprintf("PropertyName(%s)", str)
    | VariantIdentifier(str) => Printf.sprintf("VariantIdentifier(%s)", str)
    | SubName(str) => Printf.sprintf("SubName(%s)", str)
    | SubIdent(str) => Printf.sprintf("SubIdent(%i)", str)
    };
let path = (p: Identifier.path) =>
  CCArray.to_string(~sep=" -> ", i => identifier(i), p);

let ast_node: type t. Node.node(t) => string =
  n =>
    switch (n) {
    | Module(_) => "Module"
    | Literal(_) => "Literal"
    | GenericReference(i) =>
      Printf.sprintf(
        "GenericReference(%s)",
        Ast_generator_utils.Naming.fromIdentifier(i),
      )
    | Basic(_) => "Basic"
    | TypeDeclaration({name, _}) =>
      Printf.sprintf("TypeDeclaration(%s)", identifier(name))
    | Array(_) => "Array"
    | Optional(_) => "Optional"
    | Nullable(_) => "Nullable"
    | Reference({target, _}) =>
      Printf.sprintf(
        "Reference(%s)",
        Ast_generator_utils.Naming.full_identifier_of_path(target),
      )
    | Variant(_) => "Variant"
    | Fixture(_) => "Fixture"
    | Tuple(_) => "Tuple"
    | Function(_) => "Function"
    | Parameter({name, _}) =>
      Printf.sprintf("Parameter(%s)", identifier(name))
    | Record(_) => "Record"
    };