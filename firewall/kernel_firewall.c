#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>

static struct nf_hook_ops nfho;
static unsigned int block_ip = 0xC0A80164; // Default: 192.168.1.100


MODULE_LICENSE("GPL");
MODULE_AUTHOR("phantomsoldierking");
MODULE_VERSION("1.0");
module_param(block_ip, uint, 0);
MODULE_PARM_DESC(block_ip, "IP address to block (in hex, network byte order)");

/* Hook function */
unsigned int firewall_hook(void *priv, struct sk_buff *skb,
                           const struct nf_hook_state *state) {
    struct iphdr *ip_header;

    if (!skb)
        return NF_ACCEPT;

    ip_header = ip_hdr(skb);
    if (!ip_header)
        return NF_ACCEPT;

    if (ip_header->saddr == htonl(block_ip)) {
        printk(KERN_INFO "KernelFirewall: Dropped packet from %pI4\n", &ip_header->saddr);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

/* Module initialization */
static int __init firewall_init(void) {
    nfho.hook = firewall_hook;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &nfho);
    printk(KERN_INFO "Kernel Firewall loaded. Blocking IP: %pI4\n", &block_ip);
    return 0;
}

/* Module cleanup */
static void __exit firewall_exit(void) {
    nf_unregister_net_hook(&init_net, &nfho);
    printk(KERN_INFO "Kernel Firewall unloaded.\n");
}

module_init(firewall_init);
module_exit(firewall_exit);
