%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(tao_ic).

-export([gen/1, gen/2, version/0]).

-include("tao_ic.hrl").

gen(File) ->
    gen(File, []).

gen(File, Opts) ->
    gen0(map_options(Opts) ++ [to_list(File)]).

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
    string:join([?COMMAND|Opts], " ").

map_options(Opts) ->
    lists:map(fun map_option/1, Opts).

map_option(Opt) ->
    case Opt of
        % frontend options
        {local_escape, Esc} ->
            string:concat("-A", Esc);
        {dump, true} ->
            "-d";
        {preproc_cmd, Cmd} ->
            string:concat("-Yp,", Cmd);
        {preproc_flags, Flags} ->
            string:concat("-Wp,", string:join(Flags, ","));
        {preproc_to_stdout, true} ->
            "-E";
        {include, Dir} ->
            string:concat("-I", Dir);
        {define, Name} ->
            string:concat("-D", Name);
        {define, Name, Value} ->
            string:concat("-D", Name) ++ string:concat("=", Value);
        {undef, Name} ->
            string:concat("-U", Name);
        {trace, true} ->
            "-v";
        {silent, true} ->
            "-w";
        {warn_case, true} ->
            "-Cw";
        {error_case, true} ->
            "-Ce";
        {tempdir, Dir} ->
            string:concat("-t ", Dir);

        % backend options
        {be, T} ->
            "-Wb," ++
            case T of
                {port_driver_name, Name} ->
                    string:concat("port_driver_name=", Name);
                {skel_export_macro, Macro} ->
                    string:concat("skel_export_macro=", Macro);
                {skel_export_include, Include} ->
                    string:concat("skel_export_include=", Include);
                {stub_export_macro, Macro} ->
                    string:concat("stub_export_macro=", Macro);
                {stub_export_include, Include} ->
                    string:concat("stub_export_include=", Include);
                _Else ->
                    throw({invalid_be_option, T})
            end;
        {suppress_skel, true} ->
            "-SS";
        {otp, true} ->
            "-otp";
        {outputdir, Dir} ->
            string:concat("-o ", Dir);

        {_A, false} ->
            ""; % ignore option
        _Else ->
            throw({invalid_option, Opt})
    end.

to_list(A) when is_atom(A) ->
    atom_to_list(A);
to_list(L) when is_list(L) ->
    L.
