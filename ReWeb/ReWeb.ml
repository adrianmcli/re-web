(** ReWeb - an ergonomic web framework. Based on the design proposed in
    {{: https://gist.github.com/yawaramin/f0a24f1b01b193dd6d251e5e43be65e9}} *)

module Body = Body

module Headers = Httpaf.Headers

module Manual = Manual
(** ReWeb user's manual *)

module Request = Request
(** Read requests. *)

module Response = Response
(** Send responses. *)

module Server = Server
(** Create and serve endpoints. *)
