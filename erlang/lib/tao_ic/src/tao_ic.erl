%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(tao_ic).

-export([gen/1, gen/2, version/0]).

-include("tao_ic.hrl").

gen(File) ->
    gen(File, []).

gen(File, Opts) ->
    gen0(map_options(Opts) ++ [lists@:to_list(File)]).

version() ->
    gen0(["-V"]).

%%----------------------------------------------------------------------
%% Internal functions.
%%----------------------------------------------------------------------

gen0(Opts) ->
    loop(open_port({spawn, command(Opts)},
                   [exit_status, in, {line, ?LINE_MAX}, stderr_to_stdout])).

loop(Port) ->
    receive
        {Port, {exit_status, 0}} ->
            ok;
        {Port, {exit_status, _Status}} ->
            error; % non-zero exit status
        {Port, {data, {_Flag, Line}}} ->
            io:format("~s~n", [Line]),
            loop(Port) % tail-recursive
    end.

command(Opts) ->
    Cmd = os:find_executable(?COMMAND),
    case Cmd of
        false ->
            error:badcmd(?COMMAND);
        _Else ->
            string:join([Cmd|Opts], " ")
    end.    

map_options(Opts) ->
    [map_option(Opt) || Opt <- Opts].

map_option(Opt) ->
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
                    error:badarg(Opt)
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
            error:badarg(Opt)
    end.
