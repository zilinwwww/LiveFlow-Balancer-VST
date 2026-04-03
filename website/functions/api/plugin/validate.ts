import { json, type Env } from '../_utils';

interface ValidateRequest { key: string; machineId: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const { key, machineId } = await context.request.json<ValidateRequest>();
    if (!key || !machineId) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);

    const license = await context.env.DB.prepare(
      'SELECT id, status, machine_id FROM licenses WHERE key = ?'
    ).bind(key).first<{ id: string; status: string; machine_id: string | null }>();

    if (!license) return json({ status: 'ok', valid: false, reason: 'INVALID_KEY' });
    if (license.status === 'revoked') return json({ status: 'ok', valid: false, reason: 'REVOKED' });
    if (license.status === 'available') return json({ status: 'ok', valid: false, reason: 'UNBOUND' });

    // status === 'bound'
    if (license.machine_id === machineId) {
      return json({ status: 'ok', valid: true });
    }
    return json({ status: 'ok', valid: false, reason: 'REBOUND' });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
