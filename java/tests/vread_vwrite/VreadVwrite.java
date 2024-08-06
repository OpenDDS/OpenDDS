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

        DDS.OctetSeqHolder bytesHolder = new DDS.OctetSeqHolder();
        ret = ts.encode_to_bytes(sample.value, bytesHolder, format);
        if (ret != DDS.RETCODE_OK.value) {
            System.out.println("ERROR: encode_to_bytes failed " + ret);
            System.exit(1);
        }

        String bytesAsString = new String(bytesHolder.value, StandardCharsets.ISO_8859_1);
        if (bytesAsString.length() != holder.value.length()) {
            System.out.println("ERROR: byte[].length " + bytesAsString.length() + " doesn't match String: " + holder.value.length());
            System.exit(1);
        }
        if (!bytesAsString.equals(holder.value)) {
            System.out.println("ERROR: byte[] " + bytesAsString + " doesn't match String: " + holder.value);
            System.exit(1);
        }

        SampleHolder sample2 = new SampleHolder();
        ret = ts.decode_from_bytes(bytesHolder.value, sample2, format);
        if (ret != DDS.RETCODE_OK.value) {
            System.out.println("ERROR: decode_from_bytes failed " + ret);
            System.exit(1);
        }

        StringHolder holder2 = new StringHolder();
        ret = ts.encode_to_string(sample2.value, holder2, format);
        if (ret != DDS.RETCODE_OK.value) {
            System.out.println("ERROR: encode_to_string (sample2) failed " + ret);
            System.exit(1);
        }
        if (!holder.value.equals(holder2.value)) {
            System.out.println("ERROR: direct to string and byte[] to sample to string don't match");
            System.exit(1);
        }
    }
}
