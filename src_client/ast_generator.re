// TODO: Implement the options from before <--- ACTUALLY: I want this AST to be as pure as possible, so all the logic will be moved to the parser.
// TODO: Implement the tests. Probably not a good idea to have JSOO run rely, but who knows
open Migrate_parsetree;
open Ast_406;
open Parsetree;
open Ast_helper;
module Util = Ast_generator_utils;

open Ast;

exception AstGeneratorException(string);

type scope = {
  path: Identifier.path,
  parent: option(Node.node(Node.Constraint.any)),
};
let defaultScope = {path: [||], parent: None};

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// --- Entry (Returns the generated OCaml AST)
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
let rec generate =
        (
          ~scope=defaultScope,
          nodes: array(Node.node(Node.Constraint.exactlyModule)),
        ) => {
  nodes
  |> CCArray.to_list
  |> CCList.fold_left(
       ((scope, struct_carry), node) => {
         let (scope, generated_struct) = print__Node__Module(~scope, node);
         (scope, struct_carry @ [generated_struct]);
       },
       (scope, []),
     );
}
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// --- Module
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
and print__Node__Module =
    (~scope, Module(node): Node.node(Node.Constraint.exactlyModule)) => {
  let module_name = Util.Naming.moduleName(node.name);
  let scope = {parent: Some(Module(node)), path: [|Module(module_name)|]};

  let (scope, generated_structure) =
    node.types
    |> CCArray.fold_left(
         ((scope, struct_carry), node) => {
           let (scope, generated_struct) =
             generate__Node__TypeDeclaration(~scope, node);
           (scope, struct_carry @ generated_struct);
         },
         (scope, []),
       );

  (scope, Util.make_module(module_name, generated_structure));
}
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// --- Type Declarations
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
and generate__Node__TypeDeclaration =
    (~scope, node: Node.node(Node.Constraint.moduleLevel)) => {
  switch (node) {
  | Fixture(_) as fixtureNode => generate__Fixture(~scope, fixtureNode)
  | TypeDeclaration(node) =>
    let type_name =
      Identifier.TypeName(
        switch (node.name) {
        | TypeName(str) => str
        },
      );
    let scope = {
      parent: Some(TypeDeclaration(node)),
      path: CCArray.append(scope.path, [||]),
    };

    switch (node.annot) {
    | Variant(variant_members) =>
      // TODO: Use normal variant type for the time being, this need to change in several levels of complexity
      // Each member could have been assiged another literal value (or not), there could be computated values
      // Also TODO: There will be more types like this that need to be parsed here as some can't return a core_type
      let type_kind =
        Util.make_variant_kind(
          variant_members
          |> CCArray.map(member =>
               Util.Naming.fromIdentifier(
                 Identifier.VariantIdentifier(
                   switch (member.VariantConstructor.name) {
                   | VariantIdentifier(str) => str
                   },
                 ),
               )
             )
          |> CCArray.to_list,
        );
      (
        scope,
        [
          Util.make_type_declaration_of_kind(
            ~aliasName=Util.Naming.fromIdentifier(type_name),
            ~kind=type_kind,
          ),
        ],
      );
    | Record(parameters) =>
      let (scope, fields) =
        CCArray.fold_left(
          (
            (scope, params),
            Node.Parameter({name, is_optional, type_, named}):
              Node.node(Node.Constraint.exactlyParameter),
          ) => {
            let (scope, t) =
              generate__Node__Assignable_CoreType(~scope, type_);
            let name_original = Util.Naming.unwrap(name);
            let name_reason = Util.Naming.fromIdentifier(name);
            let param = (
              name_reason,
              t |> CCOpt.value(~default=Util.make_type_constraint("any")),
              name_reason != name_original
                ? [Util.make_bs_as_attribute(name_original)] : [],
            );
            (scope, CCArray.append(params, [|param|]));
          },
          (scope, [||]),
          parameters,
        );
      let record_kind = Util.make_record_kind(fields);
      (
        scope,
        [
          Util.make_type_declaration_of_kind(
            ~aliasName=Util.Naming.fromIdentifier(type_name),
            ~kind=record_kind,
          ),
        ],
      );

    | annot =>
      let (scope, annotated_type) =
        generate__Node__Assignable_CoreType(~scope, annot);

      (
        scope,
        switch (annotated_type) {
        | None => []
        | Some(annotated_type) => [
            Util.make_type_declaration(
              ~aliasName=Util.Naming.fromIdentifier(type_name),
              ~aliasType=annotated_type,
            ),
          ]
        },
      );
    };
  // extracted_nodes:
  // params: array(Node.node(Node.Constraint.exactlyTypeParameter)),
  // Util.make
  };
}
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// --- Generate Assignable Node (As Core Type)
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
and generate__Node__Assignable_CoreType =
    (~scope, node: Node.node(Node.Constraint.assignable)) => {
  let return_basic = (~scope=scope, ~inner=?, t: string) => (
    scope,
    Some(Util.make_type_constraint(~inner?, t)),
  );

  switch (node) {
  | Basic(Never) => (scope, None)
  | Basic(This) => (scope, None)
  | Basic(RelevantKeyword(keyword)) =>
    // TODO: Implement stuff like Set / Map / Object / Function
    return_basic(keyword)
  | Basic(basic) =>
    switch (basic) {
    | String => return_basic("string")
    | Number =>
      // TODO: React to configured number mode
      return_basic("float")
    | Boolean => return_basic("bool")
    | Void => return_basic("unit")
    | Any => return_basic("any")
    | Null =>
      return_basic(~inner=[Util.make_type_constraint("unit")], "Js.null")
    | Undefined =>
      return_basic(~inner=[Util.make_type_constraint("unit")], "option")
    | RelevantKeyword(_)
    | This
    | Never => raise(Failure("This case should not be reached"))
    }
  | Array(inner) =>
    let (scope, inner) = generate__Node__Assignable_CoreType(~scope, inner);
    (scope, inner |> CCOpt.map(inner => Util.make_array_of(inner)));
  | Tuple(inner) =>
    let (scope, types) =
      inner
      |> CCArray.fold_left(
           ((scope, types), t) => {
             let (scope, t) = generate__Node__Assignable_CoreType(~scope, t);
             (
               scope,
               switch (t) {
               | Some(t) => CCArray.append([|t|], types)
               | None => types
               },
             );
           },
           (scope, [||]),
         );
    (
      scope,
      CCArray.length(types) > 0 ? Some(Util.make_tuple_of(types)) : None,
    );
  | Function({return_type, parameters}) =>
    let (scope, return_type) =
      generate__Node__Assignable_CoreType(~scope, return_type);
    let (scope, parameters) =
      CCArray.fold_left(
        (
          (scope, params),
          Node.Parameter({name, is_optional, type_, named}):
            Node.node(Node.Constraint.exactlyParameter),
        ) => {
          let (scope, t) =
            generate__Node__Assignable_CoreType(~scope, type_);
          let param = (
            switch (is_optional, named) {
            | (false, true)
            | (true, _) => Some(Util.Naming.fromIdentifier(name))
            | (false, false) => None
            },
            is_optional,
            t |> CCOpt.value(~default=Util.make_type_constraint("any")),
          );
          (scope, CCArray.append([|param|], params));
        },
        (scope, [||]),
        parameters,
      );

    let parameter_count = CCArray.length(parameters);
    let parameters =
      parameter_count > 0
        ? switch (CCArray.get(parameters, parameter_count - 1)) {
          | (_, true, _) =>
            CCArray.append(
              parameters,
              [|(None, false, Util.make_type_constraint("unit"))|],
            )
          | _ => parameters
          }
        : parameters;

    (
      scope,
      Some(
        Util.make_function_type(
          parameter_count > 0
            ? parameters
            : [|(None, false, Util.make_type_constraint("unit"))|],
          return_type
          |> CCOpt.value(~default=Util.make_type_constraint("any")),
        ),
      ),
    );
  | Record(_) =>
    raise(
      Failure(
        "Error: Records can not be defined inline, they have to be extracted into their own TypeDeclaration",
      ),
    )
  | Reference({target, _}) => (
      scope,
      Some(
        Util.make_type_constraint(
          Util.Naming.full_identifier_of_path(target),
        ),
      ),
    )
  | other =>
    raise(
      AstGeneratorException(
        Printf.sprintf(
          "> Error: Type '%s' should not be handled as an Assignable_CoreType",
          Pp.ast_node(other),
        ),
      ),
    )
  };
}
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// --- Generate Fixtures
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
and generate__Fixture =
    (~scope, Fixture(fixture): Node.node(Node.Constraint.exactlyFixture)) => {
  switch (fixture) {
  | AnyUnboxed => (scope, Util.make_any_helper_unboxed())
  };
};
