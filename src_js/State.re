exception Unexpected_worker_result;

// --- Global
let language = Recoil.atom({key: "language", default: Bridge_bs.Reason});
let ts_source = Recoil.atom({key: "ts_source", default: None});

let config = Recoil.atom({key: "config", default: Sync.config});
let config_open = Recoil.atom({key: "config_open", default: false});
let examples_open = Recoil.atom({key: "examples_open", default: false});

// --- Examples / Files
let source_salt = Recoil.atom({key: "source_salt", default: 0.});
type selected = {
  file: option(string),
  example: option(string),
};
let selected =
  Recoil.atom({
    key: "selected",
    default: {
      file: None,
      example: None,
    },
  });
let selected_file =
  Recoil.selectorWithWrite({
    key: "selected_file",
    get: ({get}) => get(selected).file,
    set: ({get, set}, value) =>
      if (get(selected).file != value) {
        set(source_salt, Js.Math.random());
        set(ts_source, None);
        set(selected, {...get(selected), file: value});
      },
  });
let selected_example =
  Recoil.selectorWithWrite({
    key: "selected_example",
    get: ({get}) => get(selected).example,
    set: ({get, set}, value) => {
      set(selected, {...get(selected), example: value});
    },
  });

// --- Worker specific
let worker = WebWorkers.create_webworker("worker/worker.js");
let parsing_complete: ref((. Bridge_bs.worker_response) => unit) =
  ref((. _) => ());
let working = ref(false);
WebWorkers.onMessage(
  worker,
  (e: WebWorkers.MessageEvent.t) => {
    working := false;
    parsing_complete^(. WebWorkers.MessageEvent.data(e));
  },
);
let parse_result: Recoil.readOnly(Bridge_bs.parse_result) =
  Recoil.asyncSelector({
    key: "parse_result",
    get: ({get}) => {
      let _salt = get(source_salt);
      let content = get(ts_source);
      let language = get(language);
      let config = get(config);
      let selected_file = get(selected_file);
      Js.Promise.make((~resolve, ~reject) =>
        switch (selected_file, working^) {
        | (_, true)
        | (None, _) => reject(. Not_found)
        | (Some(path), false) =>
          parsing_complete :=
            (
              (. res) =>
                switch (res) {
                | Bridge_bs.Res_Parse(res) => resolve(. res)
                | _ => raise(Unexpected_worker_result)
                }
            );
          working := true;
          WebWorkers.postMessage(
            worker,
            Bridge_bs.Parse(
              Bridge_bs.{content, language, config, file_path: path},
            ),
          );
        }
      );
    },
  });

let example_list: Recoil.readOnly(array(Bridge_bs.example)) =
  Recoil.asyncSelector({
    key: "example_list",
    get: _ => {
      Js.Promise.make((~resolve, ~reject as _) => {
        parsing_complete :=
          (
            (. res) =>
              switch (res) {
              | Bridge_bs.Res_ExampleList(res) =>
                resolve(. res->Belt.List.toArray)
              | _ => raise(Unexpected_worker_result)
              }
          );
        working := true;
        WebWorkers.postMessage(worker, Bridge_bs.ExampleList);
      });
    },
  });

let selected_example_t =
  Recoil.selector({
    key: "selected_example_t",
    get: ({get}) => {
      let examples = get(example_list);
      let selected_example = get(selected_example);
      selected_example->Belt.Option.flatMap(path =>
        examples->Belt.Array.keep(e => e.path === path)->Belt.Array.get(0)
      );
    },
  });

// --- Config elements
let suppress_warning_for_extended_records =
  Recoil.selectorWithWrite({
    key: "suppress_warning_for_extended_records",
    get: ({get}) => get(config).suppress_warning_for_extended_records,
    set: ({get, set}, value) => {
      set(
        config,
        {...get(config), suppress_warning_for_extended_records: value},
      );
    },
  });
let omit_extended_unreferenced_records =
  Recoil.selectorWithWrite({
    key: "omit_extended_unreferenced_records",
    get: ({get}) => get(config).omit_extended_unreferenced_records,
    set: ({get, set}, value) => {
      set(
        config,
        {...get(config), omit_extended_unreferenced_records: value},
      );
    },
  });
let generate_parser =
  Recoil.selectorWithWrite({
    key: "generate_parser",
    get: ({get}) => get(config).generate_parser,
    set: ({get, set}, value) => {
      set(config, {...get(config), generate_parser: value});
    },
  });
let generate_serializer =
  Recoil.selectorWithWrite({
    key: "generate_serializer",
    get: ({get}) => get(config).generate_serializer,
    set: ({get, set}, value) => {
      set(config, {...get(config), generate_serializer: value});
    },
  });
let array_mode =
  Recoil.selectorWithWrite({
    key: "array_mode",
    get: ({get}) => get(config).array_mode,
    set: ({get, set}, value) => {
      set(config, {...get(config), array_mode: value});
    },
  });
let number_mode =
  Recoil.selectorWithWrite({
    key: "number_mode",
    get: ({get}) => get(config).number_mode,
    set: ({get, set}, value) => {
      set(config, {...get(config), number_mode: value});
    },
  });
let intersection_objects =
  Recoil.selectorWithWrite({
    key: "intersection_objects",
    get: ({get}) => get(config).intersection_mode.objects,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            intersection_mode: {
              ...c.intersection_mode,
              objects: value,
            },
          };
        },
      );
    },
  });

let intersection_unions =
  Recoil.selectorWithWrite({
    key: "intersection_unions",
    get: ({get}) => get(config).intersection_mode.unions,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            intersection_mode: {
              ...c.intersection_mode,
              unions: value,
            },
          };
        },
      );
    },
  });
let intersection_classes =
  Recoil.selectorWithWrite({
    key: "intersection_oclasses",
    get: ({get}) => get(config).intersection_mode.classes,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            intersection_mode: {
              ...c.intersection_mode,
              classes: value,
            },
          };
        },
      );
    },
  });
let intersection_functions =
  Recoil.selectorWithWrite({
    key: "intersection_objfunctions",
    get: ({get}) => get(config).intersection_mode.functions,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            intersection_mode: {
              ...c.intersection_mode,
              functions: value,
            },
          };
        },
      );
    },
  });
let intersection_other =
  Recoil.selectorWithWrite({
    key: "intersectionother",
    get: ({get}) => get(config).intersection_mode.other,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            intersection_mode: {
              ...c.intersection_mode,
              other: value,
            },
          };
        },
      );
    },
  });
let intersection_tuple_members_optional =
  Recoil.selectorWithWrite({
    key: "intersection_tuple_members_optional",
    get: ({get}) => get(config).intersection_mode.tuple_members_optional,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            intersection_mode: {
              ...c.intersection_mode,
              tuple_members_optional: value,
            },
          };
        },
      );
    },
  });
let output_type =
  Recoil.selectorWithWrite({
    key: "output_type",
    get: ({get}) => get(config).output_type,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {...c, output_type: value};
        },
      );
    },
  });
let bs_string_variant_mode =
  Recoil.selectorWithWrite({
    key: "bs_string_variant_mode",
    get: ({get}) => get(config).bucklescript_config.string_variant_mode,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            bucklescript_config: {
              ...c.bucklescript_config,
              string_variant_mode: value,
            },
          };
        },
      );
    },
  });
let bs_number_variant_mode =
  Recoil.selectorWithWrite({
    key: "bs_number_variant_mode",
    get: ({get}) => get(config).bucklescript_config.number_variant_mode,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            bucklescript_config: {
              ...c.bucklescript_config,
              number_variant_mode: value,
            },
          };
        },
      );
    },
  });
let bs_number_value =
  Recoil.selectorWithWrite({
    key: "bs_number_value",
    get: ({get}) =>
      switch (get(config).bucklescript_config.number_variant_mode) {
      | `Variant(a)
      | `PolyVariant(a)
      | `BsInline(a) => a
      },
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            bucklescript_config: {
              ...c.bucklescript_config,
              number_variant_mode:
                switch (c.bucklescript_config.number_variant_mode) {
                | `Variant(_) => `Variant(value)
                | `PolyVariant(_) => `PolyVariant(value)
                | `BsInline(_) => `BsInline(value)
                },
            },
          };
        },
      );
    },
  });
let bs_mixed_variant_mode =
  Recoil.selectorWithWrite({
    key: "bs_mixed_variant_mode",
    get: ({get}) => get(config).bucklescript_config.mixed_variant_mode,
    set: ({get, set}, value) => {
      let c = get(config);
      set(
        config,
        {
          {
            ...c,
            bucklescript_config: {
              ...c.bucklescript_config,
              mixed_variant_mode: value,
            },
          };
        },
      );
    },
  });