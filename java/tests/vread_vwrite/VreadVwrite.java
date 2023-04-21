import Mod.*;
import OpenDDS.DCPS.*;
import org.omg.CORBA.*;
import java.nio.charset.*;
import java.nio.file.*;

public class VreadVwrite {
    public static void main(String[] args) throws Exception {
        String json = new String(Files.readAllBytes(Paths.get("VreadVwriteTest.json")), Charset.defaultCharset());
        SampleTypeSupport ts = new SampleTypeSupportImpl();
        RepresentationFormat format = ts.make_format(JSON_DATA_REPRESENTATION.value);

        SampleHolder sample = new SampleHolder();
        int ret = ts.decode_from_string(json, sample, format);
        if (ret != DDS.RETCODE_OK.value) {
            System.out.println("ERROR: decode_from_string failed " + ret);
            System.exit(1);
        }

        StringHolder holder = new StringHolder();
        ret = ts.encode_to_string(sample.value, holder, format);
        if (ret != DDS.RETCODE_OK.value) {
            System.out.println("ERROR: encode_to_string failed " + ret);
            System.exit(1);
        }

        String normalized = json.replaceAll("\\s", "");
        if (!normalized.equals(holder.value)) {
            System.out.println("ERROR: round-trip JSON doesn't match: " + holder.value);
            System.exit(1);
        }
    }
}
