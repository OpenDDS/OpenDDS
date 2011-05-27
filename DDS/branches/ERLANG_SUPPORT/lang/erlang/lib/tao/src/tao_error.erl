%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(tao_error).

-export([badarg/1, badcmd/1]).

badarg(Arg) ->
    throw({badarg, Arg}).

badcmd(Cmd) ->
    throw({badcmd, Cmd}).
