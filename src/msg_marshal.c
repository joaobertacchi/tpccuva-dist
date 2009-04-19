#include <rpc/xdr.h>
#include "../include/tpcc.h"
#include "msg_marshal.h"

bool_t xdr_stock_level(XDR *xdrsp, struct tstock_level_men *stock_level){
	if (xdr_int(xdrsp, &stock_level->w_id) != 1)
		return (FALSE);
	if (xdr_int(xdrsp, &stock_level->d_id) != 1)
		return (FALSE);
	if (xdr_int(xdrsp, &stock_level->threshold) != 1)
		return (FALSE);
	return (TRUE);
}

/* TODO: confirm type of time_t */
bool_t xdr_time_t(XDR *xdrsp, time_t *seg){
        if (xdr_long(xdrsp, (long*) seg) != 1)
                return (FALSE);
        return (TRUE);
}

bool_t xdr_delivery(XDR *xdrsp, struct tdelivery_men *delivery){
	if (xdr_time_t(xdrsp, &delivery->seg) != 1)
		return (FALSE);
	if (xdr_u_short(xdrsp, &delivery->mseg) != 1)
		return (FALSE);
        if (xdr_int(xdrsp, &delivery->w_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &delivery->d_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &delivery->o_carrier_id) != 1)
                return (FALSE);
	return (TRUE);
}

bool_t xdr_payment(XDR *xdrsp, struct tpayment_men *payment){
        if (xdr_int(xdrsp, &payment->w_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &payment->d_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &payment->c_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &payment->c_w_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &payment->c_d_id) != 1)
                return (FALSE);
        if (xdr_vector(xdrsp, payment->c_last, 16, sizeof(char), (const xdrproc_t) xdr_char) != 1)
                return (FALSE);
        if (xdr_double(xdrsp, &payment->h_amount) != 1)
                return (FALSE);
	return (TRUE);
}

bool_t xdr_ostatus(XDR *xdrsp, struct torder_status_men *ostatus){
        if (xdr_int(xdrsp, &ostatus->w_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &ostatus->d_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &ostatus->c_id) != 1)
                return (FALSE);
	if (xdr_vector(xdrsp, ostatus->c_last, 16, sizeof(char), (const xdrproc_t) xdr_char) != 1)
		return (FALSE);
	return (TRUE);
}

bool_t xdr_articulo(XDR *xdrsp, struct articulo *art){
        if (xdr_int(xdrsp, &art->ol_i_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &art->ol_supply_w_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &art->ol_quantity) != 1)
                return (FALSE);
        if (xdr_char(xdrsp, &art->flag) != 1)
                return (FALSE);
	return (TRUE);
}

bool_t xdr_new_order(XDR *xdrsp, struct tnew_order_men *no){
	if (xdr_int(xdrsp, &no->w_id) != 1)
		return (FALSE);
        if (xdr_int(xdrsp, &no->d_id) != 1)
                return (FALSE);
        if (xdr_int(xdrsp, &no->c_id) != 1)
                return (FALSE);
	if (xdr_vector(xdrsp, (char *) no->item, 15, sizeof(struct articulo), (const xdrproc_t) xdr_articulo) != 1)
		return (FALSE);
	return (TRUE);
}

/* TODO: confirm type of key_t */
bool_t xdr_key_t(XDR *xdrsp, key_t *key){
	if (xdr_long(xdrsp, (long*) key) != 1)
                return (FALSE);
	return (TRUE);
}

bool_t xdr_msgtrm(XDR *xdrsp, struct tmsgtrm *trm){
	if (xdr_int(xdrsp, &trm->codctl) != 1)
		return (FALSE);
	if (xdr_key_t(xdrsp, &trm->sem_llave) != 1)
		return (FALSE);
	if (xdr_key_t(xdrsp, &trm->sem_ident) != 1)
                return (FALSE);
	if (xdr_key_t(xdrsp, &trm->shm_llave) != 1)
                return (FALSE);
	return (TRUE);
}

struct xdr_discrim choices[6] = {
//        {MSGTRM, (const xdrproc_t) xdr_msgtrm},
        {NEW_ORDER, (const xdrproc_t) xdr_new_order},
        {ORDER_STATUS, (const xdrproc_t) xdr_ostatus},
        {PAYMENT, (const xdrproc_t) xdr_payment},
        {DELIVERY, (const xdrproc_t) xdr_delivery},
        {STOCK_LEVEL, (const xdrproc_t) xdr_stock_level},
        {__dontcare__, NULL}
};

bool_t xdr_mensaje(XDR *xdrsp, struct mensaje *msg){
	if (xdr_int(xdrsp, &msg->tipo) != 1)
		return (FALSE);
	if (xdr_int(xdrsp, &msg->id) != 1)
		return (FALSE);
	if (xdr_int(xdrsp, &msg->srv_id) != 1)
		return (FALSE);
	if (xdr_time_t(xdrsp, &msg->time) != 1)
		return (FALSE);
        if (xdr_union(xdrsp, &msg->tipo, (char *) &msg->tran, choices, NULL) != 1)
                return (FALSE);
	return (TRUE);
}

int encode(struct mensaje *msg, char *buff, int buflen){
	XDR xdrsp;

	xdrmem_create(&xdrsp, buff, buflen, XDR_ENCODE);
        if (!xdr_mensaje(&xdrsp, msg)){
                fprintf(stderr, "Encoding failure: Mtype=%d\n", msg->tipo);
                return 0;
        }
	return 1;
}

int decode(struct mensaje *msg, char *buff, int buflen){
	XDR xdrsp;

	xdrmem_create(&xdrsp, buff, 1000, XDR_DECODE);
        if (!xdr_mensaje(&xdrsp, msg)){
                fprintf(stderr, "Decode failure!\n");
                return 0;
        }
	return 1;
}

/*
int main(){
	struct mensaje msg, msgdec;
	char buff[1000];

	msg.tipo = DELIVERY;
	msg.id = 2;
	msg.tran.delivery.seg = 1;
	msg.tran.delivery.mseg = 2;
	msg.tran.delivery.w_id = 3;
	msg.tran.delivery.d_id = 4;
	msg.tran.delivery.o_carrier_id = 5;

	if(!encode(&msg, buff, 1000))
		exit(1);
	if(!decode(&msgdec, buff, 1000))
		exit(1);

	exit(0);
}
*/
