/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS;

import android.content.Context;
import android.content.ContextWrapper;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Build;

public class AndroidNetworkCallback extends NetworkCallback {
  public void onLost(Network network) {
    NetworkConfigModifier ncm = TheServiceParticipant.network_config_modifier();
      if (ncm != null) {
        ncm.update_interfaces();
      }
    }

    public void onAvailable(Network network) {
      NetworkConfigModifier ncm = TheServiceParticipant.network_config_modifier();
        if (ncm != null) {
          ncm.update_interfaces();
        }
    }

    public void onCapabilitiesChanged(Network network,
                                      NetworkCapabilities networkCapabilities) {
      NetworkConfigModifier ncm = TheServiceParticipant.network_config_modifier();
      if (ncm != null) {
        ncm.update_interfaces();
      }
    }

    static public void register(Context context) {
      if (Build.VERSION.SDK_INT > 29) {
        ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
        AndroidNetworkCallback anc = new OpenDDS.DCPS.AndroidNetworkCallback();
        NetworkRequest.Builder nrb = new NetworkRequest.Builder();
        cm.registerNetworkCallback(nrb.build(), anc);
      }
    }
}
