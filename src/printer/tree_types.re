type ts_path = (list(string), list(string));

/**
    Modules
 */
type ts_module = {
  m_has_declare: bool,
  m_is_namespace: bool,
  m_name: ts_identifier,
  m_types: list(ts_type),
  m_imports: list(ts_import),
  m_exports: list(string),
}
/**
    Imports
 */
and ts_module_import = {
  mi_name: string,
  mi_path: string,
  mi_resolved: option(string),
}
and ts_type_import = {
  ti_path: string,
  ti_resolved: option(string),
  ti_names: list(ts_type_import_alias),
}
and ts_type_import_alias = {
  tia_name: string,
  tia_alias: option(string),
}
and ts_import =
  | Tsi_Type(ts_type_import)
  | Tsi_Module(ts_module_import)
/**
    Type Reference
 */
and ts_type_reference = {
  tr_path: list(string),
  tr_path_resolved: option(ts_path),
  tr_parameters: list(ts_type),
}
/**
    Type Declaration
 */
and ts_type_declaration = {
  td_name: ts_identifier,
  td_type: ts_type,
  td_arguments: list(ts_type_argument),
}
and ts_type_argument = {
  tda_name: ts_identifier,
  tda_extends: option(ts_type),
  tda_default: option(ts_type),
}
and ts_field = {
  f_name: ts_identifier,
  f_type: ts_type,
  f_readonly: bool,
}
/**
    Base Types
 */
and ts_base_type =
  | String
  | Number
  | Boolean
  | Void
  | Any
  | Null
  | Undefined
/**
    Union types
 */
and ts_mixed_literal = {
  strings: list(ts_identifier),
  numbers: list(int),
  bools: list(bool),
}
and ts_union_member = {
  um_ident: string,
  um_type: ts_type,
  um_classifier: string,
}
/**
    Types
 */
and ts_type =
  | Union(list(ts_union_member))
  | MixedLiteral(ts_mixed_literal)
  | NumericLiteral(list(int))
  | StringLiteral(list(ts_identifier))
  | Enum(list(ts_identifier))
  | Base(ts_base_type)
  | Interface(list(ts_field))
  | Tuple(list(ts_type))
  | Array(ts_type)
  | Optional(ts_type)
  | Nullable(ts_type)
  | Reference(ts_type_reference)
  | TypeDeclaration(ts_type_declaration)
  | Import(ts_import)
  | Module(ts_module)
  | Arg(ts_identifier)
and ts_identifier = {
  i_value: string,
  i_ident: string,
  i_type: string,
  i_module: string,
  i_variant: string,
};