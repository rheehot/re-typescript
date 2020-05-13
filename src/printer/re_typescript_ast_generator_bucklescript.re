open Re_typescript_decode_result;
open Re_typescript_decode_config;
open Migrate_parsetree;
open Ast_406;
open Parsetree;
open Ast_helper;
open Re_typescript_ast_generator_utils;

exception BS_Decode_Error(string, string);

let rec generate_type_def = (~ctx, type_def) =>
  switch (type_def) {
  | Base(base_type) => (
      Ptype_abstract,
      Some(
        switch (base_type) {
        | String => generate_base_type("string")
        | Number =>
          switch (ctx.number_mode) {
          | Float => generate_base_type("float")
          | Int => generate_base_type("int")
          | Unboxed =>
            raise(BS_Decode_Error("Not yet implemented", "Unboxed"))
          }
        | Boolean => generate_base_type("bool")
        | Void => generate_base_type("unit")
        | Ref(ref_) => generate_base_type(ref_)
        | Any => raise(BS_Decode_Error("Not yet implemented", "Any"))
        },
      ),
    )
  | Record(fields) => (
      generate_record_kind(
        fields
        |> List.map(
             fun
             | RecordField((name, _), type_, _) => (
                 name,
                 switch (type_ |> generate_type_def(~ctx)) {
                 | (_, None) =>
                   raise(
                     BS_Decode_Error(
                       "Invalid record field type",
                       BatPervasives.dump(type_),
                     ),
                   )
                 | (_, Some(t)) => t
                 },
               )
             | d =>
               raise(
                 BS_Decode_Error(
                   "Record only accepts children of type RecordField",
                   BatPervasives.dump(d),
                 ),
               ),
           ),
      ),
      None,
    )
  | RecordField(_) =>
    raise(
      BS_Decode_Error(
        "RecordField is not valid outside of type Record",
        BatPervasives.dump(type_def),
      ),
    )
  | TypeDeclaration(_) =>
    raise(
      BS_Decode_Error(
        "TypeDeclaration is not valid outside of root",
        BatPervasives.dump(type_def),
      ),
    )
  | t =>
    raise(BS_Decode_Error("Not yet implemented", BatPervasives.dump(t)))
  };

let generate = (~ctx, type_defs) => [
  Str.type_(
    Recursive,
    List.concat(
      type_defs
      |> List.map(type_def =>
           switch (type_def) {
           | TypeDeclaration((name, _), type_) =>
             let (kind, manifest) = generate_type_def(~ctx, type_);
             [Type.mk(~kind, ~manifest?, Location.mknoloc(name))];
           | d =>
             raise(
               BS_Decode_Error(
                 "Invalid data structure in root",
                 BatPervasives.dump(d),
               ),
             )
           }
         ),
    ),
  ),
];
