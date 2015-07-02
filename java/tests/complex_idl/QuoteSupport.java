/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import java.util.ArrayList;
import java.util.List;

import Complex_Idl.*;

/**
 * @author  Steven Stallion
 */
public class QuoteSupport {
    protected static final List<Quote> quotes =
        new ArrayList<Quote>();

    static {
        Quote quote;

        //

        quote = createQuote();

        quote.episode_name = "Full Frontal Nudity (1969)";
        quote.skit_name = "Dead Parrot";

        quote.cast_member =
            createCastMember("John Cleese", "Mr. Praline");

        quote.line = "'E's not pinin'! 'E's passed on! This parrot is no more! "
            + "He has ceased to be! 'E's expired and gone to meet 'is maker! "
            + "'E's a stiff! Bereft of life, 'e rests in peace! "
            + "If you hadn't nailed 'im to the perch 'e'd be pushing up the daisies! "
            + "'Is metabolic processes are now 'istory! 'E's off the twig! "
            + "'E's kicked the bucket, 'e's shuffled off 'is mortal coil, run down "
            + "the curtain and joined the bleedin' choir invisible! THIS IS AN EX-PARROT!";

        //

        quote = createQuote();

        quote.episode_name = "The Spanish Inquisition (1970)";
        quote.skit_name = "The Spanish Inquisition";

        quote.cast_member =
            createCastMember("Michael Palin", "Ximinez");

        quote.line = "Nobody expects the Spanish Inquisition! Our chief weapon is surprise... "
            + "surprise and fear...fear and surprise.... our two weapons are fear and surprise... "
            + "and ruthless efficiency.... Our three weapons are fear, surprise, and ruthless efficiency... "
            + "and an almost fanatical devotion to the Pope.... Our four...no... amongst our weapons.... "
            + "amongst our weaponry...are such elements as fear, surprise.... I'll come in again.";

        //

        quote = createQuote();

        quote.episode_name = "Mr. and Mrs. Brian Norris' Ford Popular (1972)";
        quote.skit_name = "Vicar/salesman";

        quote.cast_member =
            createCastMember("Terry Jones", "Mrs. S");

        quote.line = "It's funny, isn't it? How your best friend can just blow up like that? "
            + "I mean, you wouldn't think it was medically possible, would you?";
    }

    protected static void printQuote(Quote quote) {
        System.out.printf("[%s]\n", quote);

        System.out.printf(" episode_name   = %s\n", quote.episode_name);
        System.out.printf(" skit_name      = %s\n", quote.skit_name);

        System.out.printf(" cast_member    = %s (%s)\n",
            quote.cast_member.python, quote.cast_member.role);

        System.out.printf("\n%s\n\n", quote.line);
    }

    //

    private static Quote createQuote() {
        Quote quote = new Quote();

        quotes.add(quote);

        return quote;
    }

    private static CastMember createCastMember(String python, String role) {
        CastMember member = new CastMember();

        member.python = python;
        member.role = role;

        return member;
    }
}
