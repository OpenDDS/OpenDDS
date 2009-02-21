%%
%% $Id$
%%
-module(tao_ic).
-export([gen/1, gen/2, version/0]).

-include("tao_ic.hrl").

gen(File) ->
    gen(File, []).

gen(File, Opts) ->
    gen0(options(Opts) ++ [tao_util:to_list(File)]).

version() ->
    gen0(["-V"]).

%%----------------------------------------------------------------------
%% Internal functions.
%%----------------------------------------------------------------------

gen0(Opts) ->
    tao_util:exec(?COMMAND, Opts).

options(Opts) ->
    [option(Opt) || Opt <- Opts].

option(Opt) ->
    case Opt of
        {local_escape, Esc} ->
            "-A" ++ Esc;
        {dump, true} ->
            "-d";
        {preproc_cmd, Cmd} ->
            "-Yp," ++ Cmd;
        {preproc_flags, Flags} ->
            "-Wp," ++ string:join(Flags, ",");
        {preproc_to_stdout, true} ->
            "-E";
        {include, Dir} ->
            "-I" ++ Dir;
        {define, Name} ->
            "-D" ++ Name;
        {define, Name, Value} ->
            "-D" ++ Name ++ "=" ++ Value;
        {undef, Name} ->
            "-U" ++ Name;
        {trace, true} ->
            "-v";
        {silent, true} ->
            "-w";
        {warn_case, true} ->
            "-Cw";
        {error_case, true} ->
            "-Ce";
        {tempdir, Dir} ->
            "-t " ++ Dir;
        {be, T} ->
            "-Wb," ++
            case T of
                {port_driver_name, Name} ->
                    "port_driver_name=" ++ Name;
                {skel_export_macro, Macro} ->
                    "skel_export_macro=" ++ Macro;
                {skel_export_include, Include} ->
                    "skel_export_include=" ++ Include;
                {stub_export_macro, Macro} ->
                    "stub_export_macro=" ++ Macro;
                {stub_export_include, Include} ->
                    "stub_export_include=" ++ Include;
                _Else ->
                    tao_error:badarg(Opt)
            end;
        {suppress_skel, true} ->
            "-SS";
        {otp, true} ->
            "-otp";
        {outputdir, Dir} ->
            "-o " ++ Dir;

        {_A, false} ->
            ""; % ignore option
        _Else ->
            tao_error:badarg(Opt)
    end.
