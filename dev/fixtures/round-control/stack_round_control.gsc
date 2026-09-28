// Local diagnostic fixture. Does not replace either gameplay mod's entry point.
#namespace stack_round_control;

function autoexec bootstrap()
{
    level thread monitor();
}

function monitor()
{
    level endon("game_ended");
    SetDvar("stack_test_end_round", "0");
    announced = false;
    last_round = -1;
    for (;;)
    {
        wait 0.1;
        players = GetPlayers();
        if (players.size != 1 || !IsDefined(level.round_number) || !IsDefined(level.zombie_team))
            continue;
        if (!announced)
        {
            IPrintLnBold("STACK TEST: crouch + use to finish this round");
            announced = true;
        }
        if (level.round_number != last_round)
        {
            last_round = level.round_number;
            IPrintLn("STACK TEST: round " + last_round);
            SetDvar("stack_test_observed_round", last_round);
        }
        requested = GetDvarInt("stack_test_end_round") == 1;
        requested = requested || (players[0] GetStance() == "crouch" && players[0] UseButtonPressed());
        if (requested && (!IsDefined(level.stack_test_finishing) || !level.stack_test_finishing))
        {
            SetDvar("stack_test_end_round", "0");
            level thread finish_round();
        }
    }
}

function finish_round()
{
    level endon("game_ended");
    level.stack_test_finishing = true;
    current_round = level.round_number;
    IPrintLnBold("STACK TEST: finishing round " + current_round);
    // Kill spawned zombies through normal damage. Leave the round counter,
    // remaining spawn budget, and round notifications under the game's control.
    // Stop after 120 seconds if custom rules prevent a normal round transition.
    for (tick = 0; tick < 480 && level.round_number == current_round; tick++)
    {
        zombies = GetAiTeamArray(level.zombie_team);
        foreach (zombie in zombies)
        {
            if (IsDefined(zombie) && IsAlive(zombie))
                zombie DoDamage(zombie.health + 100000, zombie.origin);
        }
        wait 0.25;
    }
    level.stack_test_finishing = false;
    IPrintLn("STACK TEST: finish operation stopped at round " + level.round_number);
}
