(** ReWeb - an ergonomic web framework. Start by looking at
    {!module:Server} for an overview of the framework. See [bin/Main.re]
    for an example server. *)

module Body = Body
(** Handle request and response bodies. *)

module Client = Client
(** Make web requests. *)

module Filter = Filter
(** Transform services. *)

module Form = Form
(** Decode web forms into specified types. Think of this like JSON
    decoding. *)

module Request = Request
(** Read requests. *)

module Response = Response
(** Send responses. *)

module Server = Server
(** Create and serve endpoints. *)
